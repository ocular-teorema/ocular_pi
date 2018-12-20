#include <fstream>
#include <sstream>
#include <QtCore/QDebug>
#include <QtOpenGL/QtOpenGL>
#include <QtOpenGL/QGLWidget>

#include "cloudViewDialog.h"
#include "opengl/openGLTools.h"
#include "3d/mesh3DScene.h"
#include "rgb24Buffer.h"
#include "qSettingsSetter.h"

#include "meshLoader.h"

// FIXIT: GOOPEN
//#include "../../../restricted/applications/vimouse/faceDetection/faceMesh.h"


using namespace std;



const double CloudViewDialog::ROTATE_STEP   = 0.05;
const double CloudViewDialog::ZOOM_DIVISION = 1000.0;
const double CloudViewDialog::SHIFT_STEP    = 10.0;

const double CloudViewDialog::START_X = 0;
const double CloudViewDialog::START_Y = 0;
const double CloudViewDialog::START_Z = 1 * Grid3DScene::GRID_SIZE * Grid3DScene::GRID_STEP;



CloudViewDialog::CloudViewDialog(QWidget *parent) :
      ViAreaWidget(parent)
    , mCameraZoom(1.0)
    , mIsTracking(false)
{
    mFancyTexture = -1;

    for (int i = 0; i < Frames::MAX_INPUTS_NUMBER; i++)
    {
        mCameraTexture[i] = -1;
    }

    for (int i = 0; i < SCENE_NUMBER;i++)
    {
        mSceneControllers[i] = NULL;
    }

    /* Now lets work with UI a bit */
    mUi.setupUi(this);

    qDebug("Creating CloudViewDialog for working with OpenGL(%d.%d)",
            mUi.widget->format().majorVersion(),
            mUi.widget->format().minorVersion());


    connect(mUi.downButton,   SIGNAL(pressed()), this, SLOT(downRotate  ()));
    connect(mUi.upButton,     SIGNAL(pressed()), this, SLOT(upRotate    ()));
    connect(mUi.leftButton,   SIGNAL(pressed()), this, SLOT(leftRotate  ()));
    connect(mUi.rightButton,  SIGNAL(pressed()), this, SLOT(rightRotate ()));
    connect(mUi.centerButton, SIGNAL(pressed()), this, SLOT(resetCameraSlot ()));
    connect(mUi.zoomInButton, SIGNAL(pressed()), this, SLOT(zoomIn      ()));
    connect(mUi.zoomOutButton,SIGNAL(pressed()), this, SLOT(zoomOut     ()));


    connect(mUi.cameraTypeBox,SIGNAL(currentIndexChanged(int)),           this, SLOT(updateCameraMatrix()));

    connect(mUi.widget, SIGNAL(askParentInitialize()),                    this, SLOT(initializeGLSlot()));
    connect(mUi.widget, SIGNAL(askParentRepaint()),                       this, SLOT(repaintGLSlot()));
    connect(mUi.widget, SIGNAL(notifyParentResize(int, int)),             this, SLOT(resizeGLSlot(int, int)));

    connect(mUi.widget, SIGNAL(notifyParentMouseMove(QMouseEvent *)),     this, SLOT(childMoveEvent(QMouseEvent *)));

    connect(mUi.widget, SIGNAL(notifyParentMousePress(QMouseEvent *)),    this, SLOT(childPressEvent(QMouseEvent *)));

    connect(mUi.widget, SIGNAL(notifyParentMouseRelease(QMouseEvent *)),  this, SLOT(childReleaseEvent(QMouseEvent *)));

    connect(mUi.widget, SIGNAL(notifyParentKeyPressEvent(QKeyEvent *)),   this, SLOT(childKeyPressEvent(QKeyEvent *)));

    connect(mUi.widget, SIGNAL(notifyParentKeyReleaseEvent(QKeyEvent *)), this, SLOT(childKeyReleaseEvent(QKeyEvent *)));

    connect(mUi.widget, SIGNAL(notifyParentWheelEvent(QWheelEvent *)),    this, SLOT(childWheelEvent(QWheelEvent *)));

    mUi.frustumFarBox->setValue(fabs(START_Z * 3.0));

    /* Prepare table of visible objects*/
    mUi.treeView->setModel(&mTreeModel);
    mUi.treeView->setDragEnabled(true);
    mUi.treeView->setAcceptDrops(true);
    //mUi.treeView->setDropIndicatDraw3dParametersorShown(true);

    mUi.treeView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    mUi.treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);

    mUi.treeView->setColumnWidth(TreeSceneModel::NAME_COLUMN,200);
    mUi.treeView->setColumnWidth(TreeSceneModel::FLAG_COLUMN,30);
    mUi.treeView->setColumnWidth(TreeSceneModel::PARAMETER_COLUMN,20);

    connect(mUi.treeView, SIGNAL(clicked(const QModelIndex &)), &mTreeModel, SLOT(clicked(const QModelIndex &)));
    connect(mUi.treeView, SIGNAL(toggleInitiated()), this, SLOT(toggledVisibility()));

    connect(&mTreeModel, SIGNAL(modelChanged()), mUi.widget, SLOT(update()));

    connect(mUi.loadMeshPushButton, SIGNAL(released()), this, SLOT(loadMesh()));
    connect(mUi.addFramePushButton, SIGNAL(released()), this, SLOT(addCoordinateFrame()));

    addSubObject("grid"  , QSharedPointer<Scene3D>(new Grid3DScene()));
    addSubObject("plane" , QSharedPointer<Scene3D>(new Plane3DScene()));


    QSharedPointer<CoordinateFrame> worldFrame = QSharedPointer<CoordinateFrame>(new CoordinateFrame());

#if 0
/*    QSharedPointer<Scene3D> grid  = QSharedPointer<Scene3D>(new Grid3DScene());
    QSharedPointer<Scene3D> plane = QSharedPointer<Scene3D>(new Plane3DScene());
    grid->name  = "Grid";
    plane->name = "Plane";
    worldFrame->mChildren.push_back(grid);
    worldFrame->mChildren.push_back(plane);*/
    QSharedPointer<Scene3D> camera = QSharedPointer<Scene3D>(new CameraScene());
    camera->name = "camera";
    worldFrame->mChildren.push_back(camera);

    /* Test mesh load */
        Mesh3DScene *mesh = new Mesh3DScene();
        PLYLoader loader;
        const char *testMeshName = "data/box.ply";
        ifstream file(testMeshName, ios::in);
        if (!file.fail() && loader.loadPLY(file, *mesh) != 0)
        {
            qDebug() << "CloudViewDialog::Unable to load mesh " << testMeshName;
        } else {
            mesh->name = "box-ply";
        //    addSubObject("box-ply", QSharedPointer<Scene3D>(mesh));
        }
        file.close();
        worldFrame->mChildren.push_back(QSharedPointer<Scene3D>(mesh));
    /* Test colored mesh */
        Mesh3DScene *meshTest = new Mesh3DScene();
        meshTest->fillTestScene();
        meshTest->name = "Colored mesh";
        worldFrame->mChildren.push_back(QSharedPointer<Scene3D>(meshTest));


    Mesh3DScene *box = new Mesh3DScene();
    box->addAOB(Vector3dd(0.0,0.0,0.0), Vector3dd(1.0,1.0,1.0));
    box->name = "box-mesh";
    //addSubObject("box-mesh", QSharedPointer<Scene3D>(box));
    worldFrame->mChildren.push_back(QSharedPointer<Scene3D>(box));
#endif

    addSubObject("World Frame", worldFrame);

}



void CloudViewDialog::addMesh(QString name, Mesh3D *mesh)
{
    std::stringstream ss;
    mesh->dumpPLY(ss);
    Mesh3DScene *scene = new Mesh3DScene();
    PLYLoader loader;
    loader.loadPLY(ss, *scene);

    cout << "Loaded mesh:" << endl;
    cout << " Edges   :" << scene->edges.size() << endl;
    cout << " Vertexes:" << scene->vertexes.size() << endl;
    cout << " Faces   :" << scene->faces.size() << endl;
    cout << " Bounding box " << scene->getBoundingBox() << endl;

    addSubObject(name, QSharedPointer<Scene3D>((Scene3D*)scene));
}

TreeSceneController * CloudViewDialog::addSubObject (QString name, QSharedPointer<Scene3D> scene, bool visible)
{
    qDebug() << "Adding object" << name;

    TreeSceneController * result = mTreeModel.addObject(name, scene, visible);
    mUi.widget->updateGL();
    return result;
}


CloudViewDialog::~CloudViewDialog()
{
}

void CloudViewDialog::setCollapseTree(bool collapse)
{
    QList<int> sizes;
    if (collapse) {
        sizes << 1 << 0;
    } else {
        sizes << 1 << 1;
    }
        mUi.splitter->setSizes(sizes);
}

void CloudViewDialog::downRotate()
{
    mCamera *= Matrix33::RotationX(-ROTATE_STEP);
    mUi.widget->updateGL();
}

void CloudViewDialog::upRotate()
{
    mCamera *= Matrix33::RotationX( ROTATE_STEP);
    mUi.widget->updateGL();
}

void CloudViewDialog::leftRotate()
{
    mCamera *= Matrix33::RotationY( ROTATE_STEP);
    mUi.widget->updateGL();
}

void CloudViewDialog::rightRotate()
{
    mCamera *= Matrix33::RotationY(-ROTATE_STEP);
    mUi.widget->updateGL();
}

void CloudViewDialog::setZoom(double value)
{
    mCameraZoom = value;
    mUi.zoomLabel->setText(QString("x %1").arg(mCameraZoom));
}

void CloudViewDialog::zoom(int delta)
{
    setZoom(mCameraZoom * exp(delta / CloudViewDialog::ZOOM_DIVISION));
    resetCamera();
    mUi.widget->updateGL();
}

void CloudViewDialog::zoomIn()
{
    zoom(+1);
}

void CloudViewDialog::zoomOut()
{
    zoom(-1);
}

void CloudViewDialog::childWheelEvent ( QWheelEvent * event )
{
    double delta = event->delta();
    zoom(delta);
}



void CloudViewDialog::resetCameraPos()
{
    switch(mUi.cameraTypeBox->currentIndex())
    {
        case ORTHO_TOP:
            mCamera = Matrix44(Matrix33::RotationX(degToRad(90)), Vector3dd(0.0,0.0,0.0));
            break;
        case ORTHO_LEFT:
            mCamera = Matrix44(Matrix33::RotationY(degToRad(-90)), Vector3dd(0.0,0.0,0.0));
            break;
        case ORTHO_FRONT:
            mCamera = Matrix44(Matrix33(1.0), Vector3dd(0.0,0.0,0.0));
            break;
        default:
        case PINHOLE_AT_0:
            mCamera = Matrix44(Matrix33(1.0), Vector3dd( START_X, START_Y, START_Z));
            break;
        case LEFT_CAMERA:
            mCamera = Matrix44(mRectificationResult->decomposition.rotation) * Matrix44::Shift(mRectificationResult->getCameraShift());
            mCamera = mCamera.inverted();
            break;
        case RIGHT_CAMERA:
            mCamera = Matrix44(Matrix33(1.0), Vector3dd(0.0,0.0,0.0));
            break;
        case FACE_CAMERA:
        {
#if GOOPEN
            mCamera = Matrix44(Matrix33(1.0), Vector3dd(0.0,0.0,0.0));
            FaceMesh *faceMesh = static_cast<FaceMesh *>(mScenes[DETECTED_PERSON].data());
            if (faceMesh == NULL) {
                break;
            }
            printf("Setting a projection Matrix to face\n");
            Vector3dd cameraPos = faceMesh->mPerson.object.position3D;

            /* Move to zero */
            Matrix44 shiftCamToZero = Matrix44::Shift(-cameraPos);

            Matrix44 projector = shiftCamToZero;
            mCamera = Matrix44(Matrix33(1.0), -cameraPos);
#endif
        }
            break;

    }
    setZoom(1.0);
}

void CloudViewDialog::resetCameraSlot()
{
    resetCameraPos();
    resetCamera();
    qDebug() << "Resetting camera";
    mUi.widget->updateGL();
}

void CloudViewDialog::resetCamera()
{
    float farPlane = mUi.frustumFarBox->value();
    int width  = mUi.widget->width();
    int height = mUi.widget->height();
    double aspect = (double)width / (double)height;

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);

    switch(mUi.cameraTypeBox->currentIndex())
    {
        case ORTHO_TOP:
        case ORTHO_FRONT:
        case ORTHO_LEFT:
            glLoadIdentity();
            glOrtho(-width / 2.0, width / 2.0, height / 2.0, -height / 2.0, farPlane, -farPlane);
            glScaled(mCameraZoom, mCameraZoom, mCameraZoom);
         //   glRotated(90, 1.0, 0.0, 0.0);
            break;
        case PINHOLE_AT_0:
        default:
        {
            glLoadIdentity();
            OpenGLTools::gluPerspectivePosZ(60.0f, aspect, 1.0f, farPlane);
            break;
        }
        case LEFT_CAMERA:
        {
            glLoadIdentity();
            double fov = mRectificationResult->leftCamera.getVFov();
            OpenGLTools::gluPerspectivePosZ(radToDeg(fov), aspect, 1.0f, farPlane);
            break;
        }
        case RIGHT_CAMERA:
        {
            glLoadIdentity();
            double fov = mRectificationResult->rightCamera.getVFov();
            OpenGLTools::gluPerspectivePosZ(radToDeg(fov), aspect, 1.0f, farPlane);
            break;
        }
        case FACE_CAMERA:
        {
#if GOOPEN
            glLoadIdentity();
            FaceMesh *faceMesh = static_cast<FaceMesh *>(mScenes[DETECTED_PERSON].data());
            if (faceMesh == NULL)
            {
                OpenGLTools::gluPerspective(60.0f, aspect, 1.0f, farPlane);
                break;
            }
            //printf("Setting a projection Matrix to face\n");

            Vector3dd cameraPos = faceMesh->mPerson.object.position3D;
            printf("Camera is at %lf %lf %lf\n", cameraPos.x(), cameraPos.y(), cameraPos.z());


            Vector3dd windowP  = Vector3dd(-35, 220, 0);
            Vector3dd windowX  = Vector3dd(510,   0, 0);
            Vector3dd windowY  = Vector3dd(  0, 320, 0);
            Vector3dd windowP1 = windowP + windowX + windowY;


           /* Plane3d plane = Plane3d::FromPointAndVectors(windowP, windowX, windowY);

            Vector3dd projected = plane.projectPointTo(cameraPos);
            Vector3dd dir = projected - cameraPos;*/
            double invDist = 1.0 / cameraPos.z();

            double zNear = 0.9;
            double zFar  = 2000.0;
            double zDepth = zNear - zFar;

            double zToZ = ((zFar + zNear)  / zDepth) * invDist;
            double wToZ = (2 *zFar * zNear / zDepth) * invDist;

            Matrix44 matrix (
                   1.0, 0.0 ,      0.0,  0.0,
                   0.0, 1.0 ,      0.0,  0.0,
                   0.0, 0.0 ,     zToZ, wToZ,
                   0.0, 0.0 , -invDist, 0.0
               );

            cout << "Matrix projective part:" << endl;
            cout << matrix << endl;

            /*Projecting window top corner*/
            Vector3dd relativeP = windowP  - cameraPos;
            Vector3dd zeroProj = matrix * relativeP;
            cout << "Corner 1 is:" << endl;
            cout << "  3D       "  << relativeP << endl;
            cout << "  Projected"  << zeroProj << endl;

            Vector3dd intProj  = matrix * (windowP1 - cameraPos);
            cout << "Corner 2 is:" <<  intProj << endl;

            double dx = intProj.x() - zeroProj.x();
            double dy = intProj.y() - zeroProj.y();

            printf("Size is %lf %lf\n", dx, dy);

            matrix =
                Matrix44(Matrix33::Scale3(Vector3dd(-2.0 / dx, -2.0 / dy, 1.0))) *
                Matrix44::Shift(-zeroProj.x(), -zeroProj.y(), 0) *
                matrix;


            Vector3dd testIn  = ((windowP1 + windowP) / 2.0) - cameraPos;
            Vector3dd testOut = matrix * testIn;
            cout << "Test input :" << testIn  << endl;
            cout << "Test result:" << testOut << endl;

            cout << "Final Matrix:" << endl;
            cout << matrix << endl;


            OpenGLTools::glMultMatrixMatrix44(matrix);

            glViewport(0, 0, width, height);

            break;
#endif
        }

    }

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}


void CloudViewDialog::childMoveEvent(QMouseEvent *event)
{
    uint32_t buttons = event->buttons();

    if (buttons & Qt::LeftButton)
    {
        double xs = (event->x() - mTrack.x()) / 500.0;
        double ys = (event->y() - mTrack.y()) / 500.0;

        mCamera = Matrix33::RotationY( xs) * mCamera;
        mCamera = Matrix33::RotationX(-ys) * mCamera;
    }

    if (buttons & Qt::MidButton)
    {
        double xs = (event->x() - mTrack.x()) /*/ 10.0*/;
        double ys = (event->y() - mTrack.y()) /*/ 10.0*/;

        Vector3dd shift(xs, ys, 0);

        switch (mUi.cameraTypeBox->currentIndex())
        {
            case ORTHO_TOP:
            case ORTHO_LEFT:
            case ORTHO_FRONT:
               shift *= (1.0 / mCameraZoom);
               break;
            default:
            case PINHOLE_AT_0:
            case LEFT_CAMERA:
            case RIGHT_CAMERA:
               break;
        }

        mCamera = Matrix44(Matrix33(1.0), shift) * mCamera;
    }

    if (buttons & Qt::RightButton)
    {
        double xs = (event->x() - mTrack.x()) / 10.0;
        // double ys = (event->y() - track.y()) / 10.0;
        mCamera = Matrix44(Matrix33(1.0), Vector3dd(0, 0, xs)) * mCamera;
    }


    /*xRot += (event->x() - track.x()) * 3.0;
    yRot += (event->y() - track.y()) * 3.0;*/

    mTrack = event->pos();
    mUi.widget->updateGL();
}

void CloudViewDialog::childPressEvent(QMouseEvent *event)
{
    mIsTracking = 1;
    mTrack = event->pos();
}

void CloudViewDialog::childReleaseEvent(QMouseEvent * /*event*/)
{
    mIsTracking = 0;
}

void CloudViewDialog::childKeyPressEvent( QKeyEvent * event )
{

    Vector3dd shift(0.0, 0.0, 0.0);
    Matrix33 rotate(1.0);
    switch (event->key())
    {
        case Qt::Key_W:
                shift = Vector3dd(0, 0,  -SHIFT_STEP);
            break;
        case Qt::Key_A:
                rotate = Matrix33::RotationY(0.1);
            break;
        case Qt::Key_S:
                shift = Vector3dd(0, 0, SHIFT_STEP);
            break;
        case Qt::Key_D:
                rotate = Matrix33::RotationY(-0.1);
            break;

        case Qt::Key_R:
                rotate = Matrix33::RotationZ(-0.1);
            break;
        case Qt::Key_T:
                rotate = Matrix33::RotationZ(0.1);
            break;

        case Qt::Key_Up:
                shift = Vector3dd(0, SHIFT_STEP, 0);
            break;
        case Qt::Key_Left:
                shift = Vector3dd(SHIFT_STEP, 0, 0);
            break;
        case Qt::Key_Down:
                shift = Vector3dd(0, -SHIFT_STEP, 0);
            break;
        case Qt::Key_Right:
                shift = Vector3dd(-SHIFT_STEP, 0, 0);
            break;
    }
    mCamera = Matrix44(rotate, shift) * mCamera;

    mUi.widget->updateGL();
    //cout << "childKeyPressEvent" << endl;
}

void CloudViewDialog::childKeyReleaseEvent ( QKeyEvent * /*event*/ )
{}

void CloudViewDialog::keyPressEvent( QKeyEvent * /*event*/ )
{}

void CloudViewDialog::keyReleaseEvent ( QKeyEvent * /*event*/ )
{}

void CloudViewDialog::initializeGLSlot()
{
    qDebug() << "GL Init called" << endl;
    resetCameraPos();

    mUi.widget->qglClearColor(Qt::black);
    glShadeModel(GL_FLAT);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);

    /*Prepare a fancy texture*/
    glGenTextures( 1, &mFancyTexture );
    glBindTexture( GL_TEXTURE_2D, mFancyTexture );

    const int imageWidth  = 128;
    const int imageHeight = 128;

    RGB24Buffer textureBuffer(imageHeight, imageWidth, RGBColor::Blue());
    for (int i = 1; i < 8; i++)
    {
        for (int j = 1; j < 8; j++)
        {
            AbstractPainter<RGB24Buffer>(&textureBuffer).drawCircle(i * 16, j * 16, 5, RGBColor::Yellow());
        }
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S    , GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T    , GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    /* TODO: Move this to GLTools */
    GLint oldStride;
    glGetIntegerv(GL_UNPACK_ROW_LENGTH, &oldStride);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, textureBuffer.stride);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, textureBuffer.w, textureBuffer.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, &(textureBuffer.element(0,0)));

    glPixelStorei(GL_UNPACK_ROW_LENGTH, oldStride);

    glDisable(GL_TEXTURE_2D);

    if (mTreeModel.mTopItem != NULL &&
        !mTreeModel.mTopItem->mObject.isNull())
    {
        mTreeModel.mTopItem->mObject->prepareMesh(this);
    }
}

void CloudViewDialog::repaintGLSlot()
{
    if (mUi.cameraTypeBox->currentIndex() == FACE_CAMERA)
    {
        resetCamera();
        resetCameraPos();
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    OpenGLTools::glMultMatrixMatrix44(mCamera);

//    qDebug("=== Starting draw === ");
    if (mTreeModel.mTopItem != NULL &&
        !mTreeModel.mTopItem->mObject.isNull())
    {
        mTreeModel.mTopItem->mObject->draw(this);
    }
//    qDebug("=== Ending draw === ");

    //OpenGLTools::drawOrts(10.0, 1.0);

    glDisable(GL_TEXTURE_2D);
}


void CloudViewDialog::updateCameraMatrix()
{
    resizeGLSlot(mUi.widget->width(), mUi.widget->height());
}

void CloudViewDialog::updateHelperObjects()
{
    mUi.widget->update();
}

void CloudViewDialog::resizeGLSlot(int /*width*/, int /*height*/)
{
    resetCamera();
}

void CloudViewDialog::setNewScenePointer(QSharedPointer<Scene3D> newScene, int sceneId)
{
/*    if (newScene.isNull()) {
        return;
    }*/
    if (mUi.pauseButton->isChecked())
    {
        return;
    }

    bool isFirst = (mSceneControllers[sceneId] == NULL);
    if (isFirst && !newScene.isNull())
    {
        const char *name = "internal";
        switch (sceneId) {
        case MAIN_SCENE                 : name = "3d Main";          break;
        case ADDITIONAL_SCENE           : name = "3d Additional";    break;
        case CAMERA_PAIR                : name = "Camera Pair";      break;
        case GAPER_ZONE                 : name = "Gaper zone";       break;
        case CONTROL_ZONE               : name = "Control zone";     break;
        case DETECTED_PERSON            : name = "Detected person";  break;
        case DISP_CONTROL_ZONE          : name = "Control Zone";     break;
        case DISP_PREDICTED_CONTROL_ZONE: name = "Predicted Zone";   break;
        case CLUSTER_ZONE               : name = "Cluster zone";     break;
        case CLUSTER_SWARM              : name = "Cluster swarm";    break;
        case HEAD_SEARCH_ZONE           : name = "Head search zone"; break;
        case HAND_1                     : name = "Hand 1";           break;
        case HAND_2                     : name = "Hand 2";           break;
        default:
            break;
        };

        mSceneControllers[sceneId] = addSubObject(name, newScene, true);
    }

    mSceneControllers[sceneId]->replaceScene(newScene);


    mScenes[sceneId] = newScene;
    mUi.widget->update();
}

void CloudViewDialog::setNewRectificationResult (QSharedPointer<RectificationResult> rectificationResult)
{
    if (rectificationResult.isNull())
    {
        return;
    }

    mRectificationResult = rectificationResult;

    StereoCameraScene *cameraModels = new StereoCameraScene(*(mRectificationResult.data()));
    Draw3dCameraParameters parameters;
    cameraModels->setParameters(&parameters);
    setNewScenePointer(QSharedPointer<Scene3D>(cameraModels), CAMERA_PAIR);

    mUi.widget->update();
}


void CloudViewDialog::setNewCameraImage (QSharedPointer<QImage> texture, int cameraId)
{
    if (texture.isNull()) {
        return;
    }

    //SYNC_PRINT(("void CloudViewDialog::setNewCameraImage () called\n"));

    if (mCameraTexture[cameraId] != (unsigned)-1)
    {
        //SYNC_PRINT(("Deleting old texture\n"));
        mUi.widget->deleteTexture(mCameraTexture[cameraId]);
        mCameraTexture[cameraId] = -1;
    }

    mCameraImage[cameraId] = texture;
    //mCameraTexture[cameraId] = mUi.widget->bindTexture(*texture.data(), GL_TEXTURE_2D, GL_RGBA);
    mUi.widget->update();
}

GLuint CloudViewDialog::texture(int cameraId)
{
    if (mCameraTexture[cameraId] == (GLuint)-1 && !mCameraImage[cameraId].isNull())
    {
            mCameraTexture[cameraId] = mUi.widget->bindTexture(*mCameraImage[cameraId].data(), GL_TEXTURE_2D, GL_RGBA);
    }

    return mCameraTexture[cameraId];
}

void CloudViewDialog::savePointsPCD()
{
    static int count = 0;

    qDebug("Dump slot called...\n");
    if (mScenes[MAIN_SCENE].isNull())
    {
        return;
    }

    char name[100];
    snprintf2buf(name, "exported%d.pcd", count);

    count++;
    qDebug("Dumping current scene to <%s>...", name);
    fstream file(name, fstream::out);
    mScenes[MAIN_SCENE]->dumpPCD(file);
    file.close();
    qDebug("done\n");
}

void CloudViewDialog::savePointsPLY()
{
    static int count = 0;

    qDebug("Dump slot called...\n");
    if (mScenes[MAIN_SCENE].isNull())
    {
        return;
    }

    char name[100];
    snprintf2buf(name, "exported%d.ply", count);

    count++;
    qDebug("Dumping current scene to <%s>...", name);
    fstream file(name, fstream::out);
    mScenes[MAIN_SCENE]->dumpPLY(file);
    file.close();
    qDebug("done\n");
}

void CloudViewDialog::toggledVisibility()
{
    QItemSelectionModel *selection = mUi.treeView->selectionModel();
    QModelIndexList	list = selection->selectedRows();
    for (const QModelIndex &index : list)
    {
        qDebug() << "Model Ids:" << index.row();
        QModelIndex boxIndex = index.sibling(index.row(), TreeSceneModel::FLAG_COLUMN);
        bool isVisible = mTreeModel.data(boxIndex, Qt::CheckStateRole).toBool();
        mTreeModel.setData(boxIndex, !isVisible, Qt::CheckStateRole);
    }

}

void CloudViewDialog::loadMesh()
{
    MeshLoader loader;

    QString type = QString("3D Model (%1)").arg(loader.extentionList().c_str());

    QString fileName = QFileDialog::getOpenFileName(
      this,
      tr("Load 3D Model"),
      ".",
      type);

    Mesh3DScene *mesh = new Mesh3DScene();

    if (!loader.load(mesh, fileName.toStdString()))
    {
        delete_safe(mesh);
           return;
        }
    QFileInfo fileInfo(fileName);
    addSubObject(fileInfo.baseName(), QSharedPointer<Scene3D>((Scene3D*)mesh));
}

void CloudViewDialog::addCoordinateFrame()
{
    QSharedPointer<CoordinateFrame> newFrame = QSharedPointer<CoordinateFrame>(new CoordinateFrame());
    addSubObject("New Frame", newFrame);
}


void CloudViewDialog::saveParameters()
{
    qDebug() << "Saving cloudview state";
    SettingsSetter visitor("cloud.ini", "root");

    mTreeModel.mTopItem->accept(visitor);

  /*  for (unsigned i = 0; i < mControllers.size(); i++)
    {
        if (mControllers[i] == NULL)
            continue;
        visitor.settings()->beginGroup(QString("Sub-object%1").arg(i));
            mControllers[i]->accept(visitor);
        visitor.settings()->endGroup();
    }*/
}

void CloudViewDialog::loadParameters()
{
    qDebug() << "Loading cloudview state";
    SettingsGetter visitor("cloud.ini", "root");

    mTreeModel.mTopItem->accept(visitor);

    /*for (unsigned i = 0; i < mControllers.size(); i++)
    {
        if (mControllers[i] == NULL)
            continue;

        visitor.settings()->beginGroup(QString("Sub-object%1").arg(i));
            mControllers[i]->accept(visitor);
        visitor.settings()->endGroup();
    }*/
//    QSettings settings("cloud.ini", QSettings::IniFormat);
}

