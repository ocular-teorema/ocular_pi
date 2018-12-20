#include "boardAligner.h"

#include <cassert>
#include <fstream>

#include "abstractPainter.h"
#include "homographyReconstructor.h"

BoardAligner::BoardAligner(BoardAlignerParams params) : BoardAlignerParams(params), generator(FillGenerator(params))
{
}

BoardAligner::BoardAligner(BoardAlignerParams params, const std::shared_ptr<CirclePatternGenerator> &sharedGenerator)
    : BoardAlignerParams(params), generator(sharedGenerator)
{
    CORE_ASSERT_TRUE_S(sharedGenerator);
}

CirclePatternGenerator* BoardAligner::FillGenerator(const BoardAlignerParams &params)
{
    CirclePatternGenerator* generator = new CirclePatternGenerator();
    int N = (int)params.boardMarkers.size();
    for (int i = 0; i < N; ++i)
        generator->addToken(i, params.boardMarkers[i].circleRadius, params.boardMarkers[i].circleCenters);
    return generator;
}

bool BoardAligner::align(DpImage &img)
{
    if (!bestBoard.size() || !bestBoard[0].size())
        return false;
    observationList.clear();
    // used in 4/5 cases; means nothing in 5th
    fixOrientation();
    bool result = false;
    switch(type)
    {
        case AlignmentType::FIT_ALL:
           result = alignDim(img,  true,  true);
           break;
        case AlignmentType::FIT_WIDTH:
           result = alignDim(img,  true, false);
           break;
        case AlignmentType::FIT_HEIGHT:
           result = alignDim(img, false,  true);
           break;
        case AlignmentType::FIT_MARKER_ORIENTATION:
           result = alignSingleMarker(img);
           break;
        case AlignmentType::FIT_MARKERS:
           result = alignMarkers(img);
           break;
    }
    return result && createList();
}

void BoardAligner::transpose()
{
    if (!bestBoard.size())
        return;

    int w = (int)bestBoard[0].size(), h = (int)bestBoard.size();
    decltype(bestBoard) board(w);
    for (auto& r: board) r.resize(h);

    for (int i = 0; i < h; ++i)
    {
        for (int j = 0; j < w; ++j)
        {
            board[j][i] = bestBoard[i][j];
        }
    }
    bestBoard = board;
}

// transpose to decrease row-wise y variance
// sort in-rows increasing x
// sort rows increasing y
void BoardAligner::fixOrientation()
{
    int w = (int)bestBoard[0].size();
    int h = (int)bestBoard.size();

    std::vector<double> sr(h), ssr(h), sc(w), ssc(w);
    for (int i = 0; i < h; ++i)
    {
        for (int j = 0; j < w; ++j)
        {
            double yv = bestBoard[i][j][1];
            sr[i] += yv;
            sc[j] += yv;
            ssr[i] += yv * yv;
            ssc[j] += yv * yv;
        }
    }

    double mr = 0.0, mc = 0.0;
    for (int i = 0; i < h; ++i)
    {
        sr[i] /= w;
        mr += ssr[i] = std::sqrt(ssr[i] / w - sr[i] * sr[i]);
    }
    for (int j = 0; j < w; ++j)
    {
        sc[j] /= h;
        mc += ssc[j] = std::sqrt(ssc[j] / h - sc[j] * sc[j]);
    }
    if (mr > mc)
        transpose();
    w = (int)bestBoard[0].size();
    h = (int)bestBoard.size();

    for (auto& r: bestBoard)
        std::sort(r.begin(), r.end(), [](const corecvs::Vector2dd &a, const corecvs::Vector2dd &b) { return a[0] < b[0]; });

    std::vector<std::pair<double, int>> ymeans(h);
    for (int i = 0; i < h; ++i)
    {
        ymeans[i].second = i;
        for (auto& v: bestBoard[i])
            ymeans[i].first += v[1];
    }
    std::sort(ymeans.begin(), ymeans.end(), [](const std::pair<double, int> &a, const std::pair<double, int> &b) { return a.first < b.first; });

    decltype(bestBoard) board;
    for (auto& p: ymeans)
        board.emplace_back(std::move(bestBoard[p.second]));
    bestBoard = board;
}

bool BoardAligner::alignDim(DpImage &img, bool fitW, bool fitH)
{
    if (!fitW && !fitH) return false;
    int w = (int)bestBoard[0].size(), h = (int)bestBoard.size();
//    std::cout << "Best board: " << w << " x " << h << "; Req: " << idealWidth << " x " << idealHeight << std::endl;
    if (fitW && fitH)
    {
        if (w != idealWidth)
        {
            std::swap(w, h);
            transpose();
        }
        if (w != idealWidth || h != idealHeight)
            return false;
    }
    else
    {
        if (fitW && bestBoard[0].size() != (size_t)idealWidth)
        {
            return false;
        }
        if (fitH && bestBoard.size() != (size_t)idealHeight)
        {
            return false;
        }
    }
//    std::cout << "Ok, continue..." << std::endl;
    corecvs::Vector2dd mean(0.0);
    for (auto& r: bestBoard)
        for (auto& c: r)
            mean += c;
    mean /= (w * h) * 0.5;

    int lb = mean[0] > img.w ? 0 : idealWidth - w;
    int tb = mean[1] > img.h ? 0 : idealHeight - h;

    classifier.resize(h);
    for (auto& row: classifier)
        row.resize(w);

    for (int i = 0; i < h; ++i)
    {

        for (int j = 0; j < w; ++j)
        {
            classifier[i][j].first = lb + j;
            classifier[i][j].second = tb + i;
            std::cout << "[" << lb + j << ", " << tb + i << "] ";
        }
        std::cout << std::endl;
    }
    return true;
}

bool BoardAligner::alignSingleMarker(DpImage &img)
{
    CORE_ASSERT_TRUE_S(boardMarkers.size() == 1);
    classify(false, img);
    return bfs();
}

bool BoardAligner::alignMarkers(DpImage &img)
{
    classify(true, img);
    return bfs();
}

bool BoardAligner::bfs()
{
    initialClassifier = classifier;
    int h = (int)classifier.size();
    int w = (int)classifier[0].size();

    int seedx = -1, seedy = -1;
    int xdx, xdy, ydx, ydy;
    int biasx, biasy;

    int corner[4][2][2] =
    {
        {{ 1, 0}, {0, 1}},
        {{ 1, 0}, {0,-1}},
        {{-1, 0}, {0, 1}},
        {{-1, 0}, {0,-1}}
    };
//    std::cout << "Best board: " << w << " x " << h << "; Req: " << idealWidth << " x " << idealHeight << std::endl;

    for (int y = 0; y < h && seedx < 0; ++y)
    {
        for (int x = 0; x < w && seedx < 0; ++x)
        {
            for (int corner_id = 0; corner_id < 4; ++corner_id)
            {
                int x1 = x + corner[corner_id][0][0];
                int y1 = y + corner[corner_id][0][1];
                int x2 = x + corner[corner_id][1][0];
                int y2 = y + corner[corner_id][1][1];

                if (x1 < 0 || x2 < 0 || y1 < 0 || y2 < 0)
                    continue;
                if (x1 >= w || x2 >= w || y1 >= h || y2 >= h)
                    continue;

                if (classifier[y1][x1].first < 0 ||
                    classifier[y ][x ].first < 0 ||
                    classifier[y2][x2].first < 0)
                    continue;
                seedx = x; seedy = y;
                biasx = classifier[y][x].first;
                biasy = classifier[y][x].second;

                int xv = classifier[y ][x ].first,
                    yv = classifier[y ][x ].second,
                    xv1= classifier[y1][x1].first,
                    yv1= classifier[y1][x1].second,
                    xv2= classifier[y2][x2].first,
                    yv2= classifier[y2][x2].second;
                xdx = corner[corner_id][0][0] * (xv1 - xv);
                ydx = corner[corner_id][0][0] * (yv1 - yv);
                xdy = corner[corner_id][1][1] * (xv2 - xv);
                ydy = corner[corner_id][1][1] * (yv2 - yv);
                break;
            }
        }
    }

    if (seedx < 0)
        return false;
    if ((xdx * xdx + xdy * xdy != 1) || (ydx * ydx + ydy * ydy != 1))
        return false;

//    std::cout << "IW: " << idealWidth << "IH: " << idealHeight << std::endl;
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            int cx = classifier[y][x].first  = (x - seedx) * xdx + (y - seedy) * xdy + biasx;
            int cy = classifier[y][x].second = (x - seedx) * ydx + (y - seedy) * ydy + biasy;

            if (cx < 0 || cx >= idealWidth || cy < 0 || cy >= idealHeight)
            {
//                std::cout << "Classifier overflow" << std::endl;
//                std::cout << "Classifier" << std::endl;
//                printClassifier(false);
                return false;
            }
            if (initialClassifier[y][x].first >= 0)
            {
                if (cx != initialClassifier[y][x].first || cy != initialClassifier[y][x].second)
                {
//                    std::cout << "Classifier mismatch" << std::endl;
//                    printClassifier(true);
//                    printClassifier(false);
                    return false;
                }
            }

        }
    }
//    std::cout << "Classifier OK" << std::endl;
    return true;
}

bool BoardAligner::createList()
{
    int h = (int)classifier.size();
    int w = (int)classifier[0].size();
    observationList.clear();
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            observationList.emplace_back(
                    corecvs::Vector3dd(
                        classifier[y][x].first * 1.0,
                        classifier[y][x].second * 1.0, 
                        0.0),
                    bestBoard[y][x]);
            if (classifier[y][x].first < 0 ||
                classifier[y][x].second < 0 ||
                classifier[y][x].first >= idealWidth ||
                classifier[y][x].second >= idealHeight)
            {
//                std::cout << "OL fail" << std::endl;
                return false;
            }
        }
    }
//    std::cout << "OL OK" << std::endl;
    return true;
}


void BoardAligner::printClassifier(bool initial)
{
    auto& c = initial ? initialClassifier : classifier;
    std::cout << (initial ? "Initial " : "") << "Classifier: " << std::endl;

    for (auto& r: c)
    {
        for (auto& p: r)
            std::cout << "[" << p.first << ", " << p.second << "] ";
        std::cout << std::endl;
    }
}

void BoardAligner::drawDebugInfo(corecvs::RGB24Buffer &buffer)
{
    if (!bestBoard.size() || !bestBoard[0].size()) return;

    int w = (int)bestBoard[0].size();
    int h = (int)bestBoard.size();
    // A: draw board
    DpImage mask(buffer.h, buffer.w);
    corecvs::RGBColor board_col = corecvs::RGBColor(0x7f0000);
    for (int i = 0; i + 1 < h; ++i)
    {
        for (int j = 0; j + 1 < w; ++j)
        {
            corecvs::Vector2dd A, B, C, D;
            A = bestBoard[i + 0][j + 0];
            B = bestBoard[i + 1][j + 0];
            C = bestBoard[i + 0][j + 1];
            D = bestBoard[i + 1][j + 1];

            corecvs::Matrix33 res, orientation, homography;

            corecvs::HomographyReconstructor c2i;
            c2i.addPoint2PointConstraint(corecvs::Vector2dd(0.0, 0.0), A);
            c2i.addPoint2PointConstraint(corecvs::Vector2dd(1.0, 0.0), B);
            c2i.addPoint2PointConstraint(corecvs::Vector2dd(0.0, 1.0), C);
            c2i.addPoint2PointConstraint(corecvs::Vector2dd(1.0, 1.0), D);

            corecvs::Matrix33 AA, BB;
            c2i.normalisePoints(AA, BB);

            homography = c2i.getBestHomographyLSE();
            homography = c2i.getBestHomographyLM(homography);
            res = BB.inv() * homography * AA;
            //double score;

            for (int i = 0; i < 1000; ++i)
            {
                for (int j = 0; j < 1000; ++j)
                {
                    corecvs::Vector3dd pt = res * Vector3dd(j * (1e-3), i * (1e-3), 1.0);
                    pt /= pt[2];

                    int xx = pt[0], yy = pt[1];
                    if (xx >= 0 && xx < buffer.w && yy >= 0 && yy < buffer.h)
                    {
                        mask.element(yy, xx) = 1.0;
                    }
                }
            }
        }
    }
    for (int i = 0; i < buffer.h; ++i)
    {
        for (int j = 0; j < buffer.w; ++j)
        {
            auto A = buffer.element(i, j);
            auto B = board_col;
            if (mask.element(i, j) > 0.0)
            {
                buffer.element(i, j) = RGBColor::lerpColor(A, B, 0.3);
            }
        }
    }

    corecvs::AbstractPainter<corecvs::RGB24Buffer> p(&buffer);
    // C: draw final classifier
    for (int i = 0; i < h; ++i)
    {
        for (int j = 0; j < w; ++j)
        {
            p.drawFormat(bestBoard[i][j].x(), bestBoard[i][j].y(), corecvs::RGBColor(0xffff00), 2, "(%d, %d)", classifier[i][j].first, classifier[i][j].second);
        }
    }
    // B: draw initial classifier
    if (initialClassifier.size())
    {
        for (int i = 0; i < h; ++i)
        {
            for (int j = 0; j < w; ++j)
            {
                if (initialClassifier[i][j].first >= 0)
                {
                    p.drawFormat(bestBoard[i][j].x(), bestBoard[i][j].y(), corecvs::RGBColor(0x00ffff), 2, "(%d, %d)", initialClassifier[i][j].first, initialClassifier[i][j].second);
                }
            }
        }
    }
}

void BoardAligner::classify(bool trackOrientation, DpImage &img)
{
//    std::cout << "CLASSIFYING" << std::endl;
    int h = (int)bestBoard.size(), w = (int)bestBoard[0].size();
    classifier.clear();
    classifier.resize(h);
    for (auto& r: classifier)
        r.resize(w, std::make_pair(-1, -1));

    std::vector<std::pair<std::pair<int,int>, double>> data;

    for (int i = 0; i + 1 < h; ++i)
    {
        for (int j = 0; j + 1 < w; ++j)
        {
            corecvs::Vector2dd A, B, C, D;
            A = bestBoard[i + 0][j + 0];
            B = bestBoard[i + 0][j + 1];
            C = bestBoard[i + 1][j + 0];
            D = bestBoard[i + 1][j + 1];
            
            double score;
            corecvs::Matrix33 orientation, res;
            DpImage query;
            int cl =  generator->getBestToken(img, {A, B, C, D}, score, orientation, res);//, query);

            if (!trackOrientation)
            {
                orientation = corecvs::Matrix33(1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0);
            }

            int order[][2] = {{0, 0}, {1, 0}, {0, 1}, {1, 1}};

            if (cl >= 0)
            {
                data.emplace_back(std::make_pair(i, j), score);
                // Take sq. orientation and place classifier
                for (int k = 0; k < 4; ++k)
                {
                    auto P = (orientation * corecvs::Vector3dd(order[k][0], order[k][1], 1.0).project()) + corecvs::Vector2dd(0.5, 0.5);
                    CORE_ASSERT_TRUE_S(P[0] > 0.0 && P[1] > 0.0);
                    CORE_ASSERT_TRUE_S(P[0] < 2.0 && P[1] < 2.0);
                    classifier[i + P[1]][j + P[0]] = std::make_pair(boardMarkers[cl].cornerX + order[k][0], boardMarkers[cl].cornerY + order[k][1]);
                }
            }
        }
    }
#if 0
    std::cout << "Scores:" << std::endl;
    for (auto& p: data)
    {
        std::cout << "[" << p.first.first << ", " << p.first.second << "]: " << p.second << std::endl;
    }
    printClassifier(false);
#endif
}
