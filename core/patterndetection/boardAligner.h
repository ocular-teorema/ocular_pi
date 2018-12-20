#ifndef BOARDALIGNER
#define BOARDALIGNER

#include <memory>

#include "vector2d.h"
#include "circlePatternGenerator.h"
#include "selectableGeometryFeatures.h"
#include "typesafeBitmaskEnums.h"

enum class AlignmentType
{
    // Fit all = fit 2 dimensions regardless of orientations
    FIT_ALL,
    // Fit width = fit width (e.g. dimension along X axis)
    FIT_WIDTH,
    // Fit height = fit height (e.g. dimension along Y)
    FIT_HEIGHT,
    // Keep orientaion and select position using central marker
    FIT_MARKER_ORIENTATION,
    // Detect orientation 
    FIT_MARKERS
//    // Detect orientation and board ids
//    FIT_MARKERS_MULTIPLE
};

struct BoardMarkerDescription
{
    int cornerX = 0, cornerY = 0;
    std::vector<corecvs::Vector2dd> circleCenters;
    double circleRadius = 0.08;
    int boardId = 0;

    template<typename VisitorType>
    void accept(VisitorType &visitor)
    {
        visitor.visit(cornerX, 0, "cornerX");
        visitor.visit(cornerY, 0, "cornerY");
        visitor.visit(circleCenters, "circleCenters");
        visitor.visit(circleRadius, 0.08, "circleRadius");
        visitor.visit(boardId, 0, "boardId");
    }

};

struct BoardAlignerParams
{
    AlignmentType type = AlignmentType::FIT_WIDTH;
    std::vector<BoardMarkerDescription> boardMarkers;
    int idealWidth = 18;
    int idealHeight = 11;

    template<typename VisitorType>
    void accept(VisitorType &visitor)
    {
        auto m = asInteger(type);
        visitor.visit(m, m, "alignmentType");
        type = static_cast<AlignmentType>(m);
        visitor.visit(idealWidth, 18, "idealWidth");
        visitor.visit(idealHeight, 11, "idealHeight");
        visitor.visit(boardMarkers, "boardMarkers");
    }

    static BoardAlignerParams GetOldBoard()
    {
        // May be we'll change default c'tor, so explicitly...
        BoardAlignerParams params;
        params.idealWidth = 18;
        params.idealHeight = 11;
        params.boardMarkers.clear();
        params.type = AlignmentType::FIT_WIDTH;
        return params;
    }

    static BoardAlignerParams GetNewBoard()
    {
        BoardAlignerParams params;
        params.idealWidth = 26;
        params.idealHeight = 18;
        params.type = AlignmentType::FIT_MARKERS;
        params.boardMarkers.clear();

        BoardMarkerDescription bmd;
        bmd.cornerX = 3;
        bmd.cornerY = 1;
        bmd.circleRadius = 0.08;
        bmd.circleCenters =
        {
            corecvs::Vector2dd(0.823379, 0.421373),
            corecvs::Vector2dd(0.441718, 0.370170),
            corecvs::Vector2dd(0.269471, 0.166728)
        };
        params.boardMarkers.push_back(bmd);

        bmd.cornerX = 2;
        bmd.cornerY = 8;
        bmd.circleCenters =
        {
            corecvs::Vector2dd(0.589928, 0.166732),
            corecvs::Vector2dd(0.269854, 0.833333),
            corecvs::Vector2dd(0.692244, 0.589654)
        };
        params.boardMarkers.push_back(bmd);

        bmd.cornerX = 12;
        bmd.cornerY = 2;
        bmd.circleCenters =
        {
            corecvs::Vector2dd(0.585422, 0.680195),
            corecvs::Vector2dd(0.287252, 0.833178),
            corecvs::Vector2dd(0.166910, 0.441706)
        };
        params.boardMarkers.push_back(bmd);

        bmd.cornerX = 21;
        bmd.cornerY = 1;
        bmd.circleCenters =
        {
            corecvs::Vector2dd(0.166911, 0.285371),
            corecvs::Vector2dd(0.421789, 0.675665),
            corecvs::Vector2dd(0.579427, 0.174676)
        };
        params.boardMarkers.push_back(bmd);

        bmd.cornerX = 22;
        bmd.cornerY = 8;
        bmd.circleCenters =
        {
            corecvs::Vector2dd(0.705211, 0.425448),
            corecvs::Vector2dd(0.167029, 0.781583),
            corecvs::Vector2dd(0.170480, 0.403242)
        };
        params.boardMarkers.push_back(bmd);

        bmd.cornerX = 21;
        bmd.cornerY = 15;
        bmd.circleCenters =
        {
            corecvs::Vector2dd(0.166779, 0.582670),
            corecvs::Vector2dd(0.422544, 0.689812),
            corecvs::Vector2dd(0.733317, 0.828926)
        };
        params.boardMarkers.push_back(bmd);

        bmd.cornerX = 12;
        bmd.cornerY = 14;
        bmd.circleCenters =
        {
            corecvs::Vector2dd(0.167587, 0.433621),
            corecvs::Vector2dd(0.686839, 0.425617),
            corecvs::Vector2dd(0.833333, 0.746159)
        };
        params.boardMarkers.push_back(bmd);

        bmd.cornerX = 3;
        bmd.cornerY = 15;
        bmd.circleCenters =
        {
            corecvs::Vector2dd(0.751807, 0.833313),
            corecvs::Vector2dd(0.441399, 0.687871),
            corecvs::Vector2dd(0.596547, 0.166670)
        };
        params.boardMarkers.push_back(bmd);
        return params;
    }
};


class BoardAligner : protected BoardAlignerParams
{
public:
    BoardAligner(BoardAlignerParams params = BoardAlignerParams());
    BoardAligner(BoardAlignerParams params, const std::shared_ptr<CirclePatternGenerator> &sharedGenerator);
    static CirclePatternGenerator* FillGenerator(const BoardAlignerParams &params);
    bool align(DpImage &img);
    void drawDebugInfo(corecvs::RGB24Buffer &buffer);
    std::vector<std::vector<corecvs::Vector2dd>> bestBoard;
protected:
    std::shared_ptr<CirclePatternGenerator> generator;
    bool sharedGenerator;
    std::vector<std::vector<std::pair<int, int>>> classifier;
    std::vector<std::vector<std::pair<int, int>>> initialClassifier;
    corecvs::ObservationList observationList;
    void printClassifier(bool initial);
private:
    bool alignDim(DpImage &img, bool fitW, bool fitH);
    bool alignSingleMarker(DpImage &img);
    bool alignMarkers(DpImage &img);
    void classify(bool trackOrientation, DpImage &img);
    void fixOrientation();
    void transpose();
    bool bfs();
    bool createList();
};

#endif
