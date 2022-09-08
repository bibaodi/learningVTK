#include "QtVTKRenderWindows.h"
#include "ui_QtVTKRenderWindows.h"
#include "vtkAxisActor.h"
#include "vtkBoundedPlanePointPlacer.h"
#include "vtkCellPicker.h"
#include "vtkCommand.h"
#include "vtkCoordinate.h"
#include "vtkDICOMImageReader.h"
#include "vtkDistanceRepresentation.h"
#include "vtkDistanceRepresentation2D.h"
#include "vtkDistanceWidget.h"
#include "vtkHandleRepresentation.h"
#include "vtkImageActor.h"
#include "vtkImageData.h"
#include "vtkImageMapToWindowLevelColors.h"
#include "vtkImageSlabReslice.h"
#include "vtkInformation.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkInteractorStyleImage.h"
#include "vtkLookupTable.h"
#include "vtkPlane.h"
#include "vtkPlaneSource.h"
#include "vtkPointHandleRepresentation2D.h"
#include "vtkPointHandleRepresentation3D.h"
#include "vtkProperty.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkResliceCursor.h"
#include "vtkResliceCursorActor.h"
#include "vtkResliceCursorLineRepresentation.h"
#include "vtkResliceCursorPolyDataAlgorithm.h"
#include "vtkResliceCursorThickLineRepresentation.h"
#include "vtkResliceCursorWidget.h"
#include "vtkResliceImageViewer.h"
#include "vtkResliceImageViewerMeasurements.h"
#include <iostream>
#include <vtkCamera.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>

#include "vtkProperty2D.h"
#include "vtkTextProperty.h"
#include <vtkLegendScaleActor.h>

//------------------------------------------------------------------------------
class vtkResliceCursorCallback : public vtkCommand {
  public:
    static vtkResliceCursorCallback *New() { return new vtkResliceCursorCallback; }

    void Execute(vtkObject *caller, unsigned long ev, void *callData) override {
        std::cout << "\033[35m*******************callback: EVENT=" << ev << " ]]]\n";
        if (ev == vtkResliceCursorWidget::WindowLevelEvent || ev == vtkCommand::WindowLevelEvent ||
            ev == vtkResliceCursorWidget::ResliceThicknessChangedEvent) {
            // Render everything
            for (int i = 0; i < 3; i++) {
                this->RCW[i]->Render();
            }
            this->IPW[0]->GetInteractor()->GetRenderWindow()->Render();
            return;
        }

        vtkImagePlaneWidget *ipw = dynamic_cast<vtkImagePlaneWidget *>(caller);
        if (ipw) {
            std::cout << "vtkImagePlaneWidget . return.\n";

            double origin[3], point1[3], point2[3];
            ipw->GetOrigin(origin);
            ipw->GetPoint1(point1);
            ipw->GetPoint2(point2);
            for (int i = 0; i < 3; i++) {
                std::cout << "origin[" << i << "=]" << origin[i] << ",pt1[" << i << "]=" << point1[i] << ",pt2[" << i
                          << "]=" << point2[i] << std::endl;
                if (nullptr == RCW[i]) {
                    std::cout << "Err: RCW nullptr \n";
                    return;
                }
                vtkResliceCursorLineRepresentation *rep =
                    dynamic_cast<vtkResliceCursorLineRepresentation *>(RCW[i]->GetResliceCursorRepresentation());
                if (nullptr == rep) {
                    std::cout << "Err: rep not cast \n";
                    return;
                }
                vtkResliceCursor *rslc = rep->GetResliceCursor();
                rslc->SetCenter(origin);
            }
            double *wl = static_cast<double *>(callData);
            if (nullptr == callData) {
                return;
            }
            std::cout << "vtkImagePlaneWidget callData=" << wl[0] << wl[1] << wl[2];

            if (ipw == this->IPW[0]) {
                this->IPW[1]->SetWindowLevel(wl[0], wl[1], 1);
                this->IPW[2]->SetWindowLevel(wl[0], wl[1], 1);
            } else if (ipw == this->IPW[1]) {
                this->IPW[0]->SetWindowLevel(wl[0], wl[1], 1);
                this->IPW[2]->SetWindowLevel(wl[0], wl[1], 1);
            } else if (ipw == this->IPW[2]) {
                this->IPW[0]->SetWindowLevel(wl[0], wl[1], 1);
                this->IPW[1]->SetWindowLevel(wl[0], wl[1], 1);
            }
        }

        vtkResliceCursorWidget *rcw = dynamic_cast<vtkResliceCursorWidget *>(caller);
        if (rcw) {
            std::cout << "Caller vtkResliceCursorWidget="
                      << "\n";
            vtkResliceCursorLineRepresentation *rep =
                dynamic_cast<vtkResliceCursorLineRepresentation *>(rcw->GetRepresentation());
            // Although the return value is not used, we keep the get calls
            // in case they had side-effects
            rep->GetResliceCursorActor()->GetCursorAlgorithm()->GetResliceCursor();
            rep->SetShowReslicedImage(1);
            // std::cout << "GetThicknessLabelPosition=" << rep->GetThicknessLabelPosition() << "\n";
            for (int i = 0; i < 3; i++) {
                vtkPlaneSource *ps = static_cast<vtkPlaneSource *>(this->IPW[i]->GetPolyDataAlgorithm());
                ps->SetOrigin(this->RCW[i]->GetResliceCursorRepresentation()->GetPlaneSource()->GetOrigin());
                ps->SetPoint1(this->RCW[i]->GetResliceCursorRepresentation()->GetPlaneSource()->GetPoint1());
                ps->SetPoint2(this->RCW[i]->GetResliceCursorRepresentation()->GetPlaneSource()->GetPoint2());

                // If the reslice plane has modified, update it on the 3D widget
                this->IPW[i]->UpdatePlacement();
            }
        }
        vtkResliceImageViewer *rsiv = dynamic_cast<vtkResliceImageViewer *>(caller);
        if (rsiv) {
            std::cout << "Caller vtkResliceImageViewer=" << caller << "\t";
            int posxy[2];
            rsiv->GetInteractor()->GetEventPosition(posxy); // the position coordinate belong to window;
            rsiv->GetInteractor()->GetPicker()->DebugOn();
            // rsiv->GetInteractor()->GetPicker()->PrintSelf(std::cout, vtkIndent(5));

            std::cout << "postion=[" << posxy[0] << "," << posxy[1] << "]\n";
            for (int i = 0; i < 3; i++) {
                vtkPlaneSource *psource = RCW[i]->GetResliceCursorRepresentation()->GetPlaneSource();
                double planeOrigin[3];
                psource->GetOrigin(planeOrigin);
                std::cout << "\nplaneOrigin[" << i << "]=" << planeOrigin[0] << ",\t" << planeOrigin[1] << ", \t"
                          << planeOrigin[2];
                std::cout << std::endl;
                bool corelationV1 = false;
                if (corelationV1) {
                    IPW[i]->SetSlicePosition(planeOrigin[i]);
                }
                bool corelationV2 = !corelationV1;
                if (corelationV2) {
                    std::cout << "\t\033[32m>>Using corelation V2<<\t\033[0m";
                    vtkPlaneSource *ipwPS = static_cast<vtkPlaneSource *>(this->IPW[i]->GetPolyDataAlgorithm());
                    ipwPS->SetOrigin(psource->GetOrigin());
                    ipwPS->SetPoint1(psource->GetPoint1());
                    ipwPS->SetPoint2(psource->GetPoint2());
                    // If the reslice plane has modified, update it on the 3D widget
                    this->IPW[i]->UpdatePlacement();
                }
            }
        }

        // Render everything
        for (int i = 0; i < 3; i++) {
            this->RCW[i]->Render();
            this->IPW[i]->GetInteractor()->GetRenderWindow()->Render();
        }
        // window-> ->RenderOverlay(RCW[1]->GetCurrentRenderer());//useless.
        window->addDistanceScaleV4(1);

        int *windowSize[3];
        windowSize[0] = this->RCW[0]->GetInteractor()->GetRenderWindow()->GetSize();
        windowSize[1] = this->RCW[0]->GetInteractor()->GetRenderWindow()->GetActualSize();
        windowSize[2] = this->RCW[0]->GetInteractor()->GetRenderWindow()->GetScreenSize();
        int *windowPos[3];
        windowPos[0] = this->RCW[0]->GetInteractor()->GetRenderWindow()->GetPosition();
        windowPos[1] = this->RCW[1]->GetInteractor()->GetRenderWindow()->GetPosition();
        windowPos[2] = this->RCW[2]->GetInteractor()->GetRenderWindow()->GetPosition();
        for (int i = 0; i < 1; i++) {
            std::cout << "size=" << windowSize[0][i] << "," << windowSize[0][i + 1] << std::endl;
            std::cout << "Asize=" << windowSize[1][i] << "," << windowSize[1][i + 1] << std::endl;
            std::cout << "Ssize=" << windowSize[2][i] << "," << windowSize[2][i + 1] << std::endl;
            std::cout << "windowPos[0]=" << windowPos[0][i] << "," << windowPos[0][i + 1] << std::endl;
            std::cout << "windowPos[1]=" << windowPos[1][i] << "," << windowPos[1][i + 1] << std::endl;
            std::cout << "windowPos[2]=" << windowPos[2][i] << "," << windowPos[2][i + 1] << std::endl;
        }
        std::cout << "\033[0m\n";
    }

    vtkResliceCursorCallback() {}
    vtkImagePlaneWidget *IPW[3];
    vtkResliceCursorWidget *RCW[3];
    QtVTKRenderWindows *window;
};

QtVTKRenderWindows::QtVTKRenderWindows(int vtkNotUsed(argc), char *argv[]) {
    this->ui = new Ui_QtVTKRenderWindows;
    this->ui->setupUi(this);

    vtkSmartPointer<vtkDICOMImageReader> reader = vtkSmartPointer<vtkDICOMImageReader>::New();
    reader->SetDirectoryName(argv[1]);
    reader->Update();
    // reader->PrintSelf(std::cout, vtkIndent(4));
    int imageDims[3];
    reader->GetOutput()->GetDimensions(imageDims);

    m_data = reader->GetOutput();
    m_data->PrintSelf(std::cout, vtkIndent(4));
    // clang-format off
    // clang-format on
    QVTKRenderWidget *views[3];
    views[0] = this->ui->view1;
    views[1] = this->ui->view2;
    views[2] = this->ui->view3;
    for (int i = 0; i < 3; i++) {
        m_riv[i] = vtkSmartPointer<vtkResliceImageViewer>::New();
        std::cout << "vtkResliceImageViewer[" << i << "] Pointer=" << m_riv[i] << "\n";
        vtkNew<vtkGenericOpenGLRenderWindow> renderWindow;
        m_riv[i]->SetRenderWindow(renderWindow);

        views[i]->setRenderWindow(m_riv[i]->GetRenderWindow());
        QPoint posLocal = views[i]->pos();
        int posX = views[i]->mapToGlobal(posLocal).x();
        int posY = views[i]->mapToGlobal(posLocal).y();
        std::cout << "View position[" << i << "]=" << posX << "," << posY << ")\n";
        m_riv[i]->SetupInteractor(views[i]->renderWindow()->GetInteractor());
        double displaypt[3];
        m_riv[i]->GetRenderer()->GetDisplayPoint(displaypt);
        // renderWindow->PrintSelf(std::cout, vtkIndent(4));
    }

    //    this->ui->view2->setRenderWindow(m_riv[1]->GetRenderWindow());
    //    m_riv[1]->SetupInteractor(this->ui->view2->renderWindow()->GetInteractor());

    //    this->ui->view3->setRenderWindow(m_riv[2]->GetRenderWindow());
    //    m_riv[2]->SetupInteractor(this->ui->view3->renderWindow()->GetInteractor());

    vtkResliceCursor *rslc = m_riv[1]->GetResliceCursor();
    bool correctImageDirection = true;
    if (correctImageDirection) {
        double direction[3] = {0, 0, 0};
        // rslc->SetXAxis(-1, 0, 0);
        // rslc->SetZAxis(0, 0, -1);
        double viewup[3] = {0, 1, 0};
        rslc->SetZViewUp(viewup);
        rslc->SetXViewUp(viewup);
        viewup[1] = 0;
        viewup[2] = -1;
        rslc->SetYViewUp(viewup);
    }
    // rslc->PrintSelf(std::cout, vtkIndent(4));

    bool usingThickNess = false;
    if (usingThickNess) {
        rslc->SetThickMode(1);
        double thickness[3] = {1, 3, 4};
        rslc->SetThickness(thickness);
        // rslc->SetHoleWidthInPixels(5.9);
    }
    for (int i = 0; i < 3; i++) {
        // make them all share the same reslice cursor object.
        vtkResliceCursorLineRepresentation *rep =
            vtkResliceCursorLineRepresentation::SafeDownCast(m_riv[i]->GetResliceCursorWidget()->GetRepresentation());
        bool usingImageActor = false;
        if (usingImageActor) {
            rep->SetUseImageActor(1);
            vtkImageActor *imgActor = rep->GetImageActor();
            imgActor->SetInputData(rslc->GetImage());
            imgActor->RotateY(45);
        }

        rep->SetRestrictPlaneToVolume(1);
        rep->SetThicknessLabelFormat("Thick=%d");

        vtkResliceCursorActor *cursorActor = rep->GetResliceCursorActor();

        cursorActor->GetCursorAlgorithm()->SetReslicePlaneNormal(i);
        for (int iii = 0; iii < 3; iii++) {
            // std::cout << "ViewUp=[" << i << "]=" << viewup[i][iii] << std::endl;
            vtkProperty *clProperty = cursorActor->GetCenterlineProperty(iii);
            clProperty->SetLineWidth(iii * 5);
        }
        // cursorActor->GetCursorAlgorithm()->SetResliceCursor(rslc);
        // cursorActor->RotateX(45); //because it is a texture in plane, so rotate is not useful.

        // rep->SetManipulationMode(2);
        int maniMode = rep->GetManipulationMode();
        std::cout << "GetManipulationMode=" << maniMode << std::endl;
        // m_riv[i]->SetResliceCursor(m_riv[0]->GetResliceCursor());
        m_riv[i]->SetResliceCursor(rslc);

        m_riv[i]->SetInputData(reader->GetOutput());
        m_riv[i]->SetSliceOrientation(i);
        m_riv[i]->SetResliceModeToAxisAligned();

        /*This will change Image size on Screen, default if Parallel projection. shoud
         not change to close.*/
        /**I want to correct slice direction --begin**/
        vtkRenderer *ren = m_riv[i]->GetRenderer();
        ren->ResetCamera();

        m_riv[i]->SetResliceModeToOblique();

        vtkCamera *cam = ren->GetActiveCamera();
        cam->SetObliqueAngles(30.f, 60.345);
        ren->ResetCamera();
        std::cout << "camera printSelf \033[36m";
        // cam->PrintSelf(std::cout, vtkIndent(4));
        std::cout << "camera printSelf \033[0m" << std::endl;

        /**I want to correct slice direction --begin.end**/

        m_riv[i]->GetResliceCursorWidget()->SetEnabled(1);
    }

    vtkSmartPointer<vtkCellPicker> picker = vtkSmartPointer<vtkCellPicker>::New();
    picker->SetTolerance(0.005);

    vtkSmartPointer<vtkProperty> ipwProp = vtkSmartPointer<vtkProperty>::New();
    vtkSmartPointer<vtkRenderer> ren = vtkSmartPointer<vtkRenderer>::New();

    vtkNew<vtkGenericOpenGLRenderWindow> renderWindow;
    this->ui->view4->setRenderWindow(renderWindow);
    this->ui->view4->renderWindow()->AddRenderer(ren);
    vtkRenderWindowInteractor *iren = this->ui->view4->interactor();

    for (int i = 0; i < 3; i++) {
        planeWidget[i] = vtkSmartPointer<vtkImagePlaneWidget>::New();
        planeWidget[i]->SetInteractor(iren);
        planeWidget[i]->SetPicker(picker);
        planeWidget[i]->RestrictPlaneToVolumeOn();
        double color[3] = {0.1, 0.1, 0.1};
        double colorB[3] = {0.1, 0.2, 0.5};
        color[i] = 1.f;
        bool usingCursorProperty = false;
        if (usingCursorProperty) {
            vtkProperty *ipwCursorProperty = planeWidget[i]->GetCursorProperty();
            ipwCursorProperty->SetLineWidth(20);
            ipwCursorProperty->SetColor(color);
        }

        color[i] = 1;
        planeWidget[i]->GetPlaneProperty()->SetColor(color);
        vtkSmartPointer<vtkProperty> textureProperty = vtkSmartPointer<vtkProperty>::New();
        textureProperty->SetColor(color);
        planeWidget[i]->SetTexturePlaneProperty(textureProperty);

        color[0] /= 4.0;
        color[1] /= 4.0;
        color[2] /= 4.0;
        m_riv[i]->GetRenderer()->SetBackground(color);

        planeWidget[i]->SetTexturePlaneProperty(ipwProp);
        planeWidget[i]->TextureInterpolateOff();
        planeWidget[i]->SetResliceInterpolateToLinear();
        planeWidget[i]->SetInputConnection(reader->GetOutputPort());
        planeWidget[i]->SetPlaneOrientation(i);
        planeWidget[i]->SetSliceIndex(imageDims[i] / 2);
        planeWidget[i]->DisplayTextOn();
        planeWidget[i]->SetDefaultRenderer(ren);
        planeWidget[i]->SetWindowLevel(1358, -27);
        planeWidget[i]->On();
        planeWidget[i]->InteractionOn();
    }

    // addDistanceScaleV4(0); will not display correct, but when scroll mouse it will display good. --eton@22/09/08

    vtkSmartPointer<vtkResliceCursorCallback> cbk = vtkSmartPointer<vtkResliceCursorCallback>::New();

    for (int i = 0; i < 3; i++) {
        cbk->IPW[i] = planeWidget[i];
        cbk->RCW[i] = m_riv[i]->GetResliceCursorWidget();
        m_riv[i]->GetResliceCursorWidget()->AddObserver(vtkResliceCursorWidget::ResliceAxesChangedEvent, cbk);
        m_riv[i]->GetResliceCursorWidget()->AddObserver(vtkResliceCursorWidget::WindowLevelEvent, cbk);
        m_riv[i]->GetResliceCursorWidget()->AddObserver(vtkResliceCursorWidget::ResliceThicknessChangedEvent, cbk);
        m_riv[i]->GetResliceCursorWidget()->AddObserver(vtkResliceCursorWidget::ResetCursorEvent, cbk);
        m_riv[i]->GetInteractorStyle()->AddObserver(vtkCommand::WindowLevelEvent, cbk);
        m_riv[i]->AddObserver(vtkResliceImageViewer::SliceChangedEvent, cbk);
        // planeWidget[i]->AddObserver(vtkCommand::AnyEvent, cbk);

        // Make them all share the same color map.
        m_riv[i]->SetLookupTable(m_riv[0]->GetLookupTable());
        planeWidget[i]->GetColorMap()->SetLookupTable(m_riv[0]->GetLookupTable());
        // planeWidget[i]->GetColorMap()->SetInput(m_riv[i]->GetResliceCursorWidget()->GetResliceCursorRepresentation()->GetColorMap()->GetInput());
        planeWidget[i]->SetColorMap(
            m_riv[i]->GetResliceCursorWidget()->GetResliceCursorRepresentation()->GetColorMap());
    }
    addScale();
    addDistanceScale(2);
    cbk->window = this;

    this->ui->view1->show();
    this->ui->view2->show();
    this->ui->view3->show();
    // Set up action signals and slots
    connect(this->ui->actionExit, SIGNAL(triggered()), this, SLOT(slotExit()));
    connect(this->ui->resliceModeCheckBox, SIGNAL(stateChanged(int)), this, SLOT(resliceMode(int)));
    connect(this->ui->thickModeCheckBox, SIGNAL(stateChanged(int)), this, SLOT(thickMode(int)));
    this->ui->thickModeCheckBox->setEnabled(0);

    connect(this->ui->radioButton_Max, SIGNAL(pressed()), this, SLOT(SetBlendModeToMaxIP()));
    connect(this->ui->radioButton_Min, SIGNAL(pressed()), this, SLOT(SetBlendModeToMinIP()));
    connect(this->ui->radioButton_Mean, SIGNAL(pressed()), this, SLOT(SetBlendModeToMeanIP()));
    this->ui->blendModeGroupBox->setEnabled(0);

    connect(this->ui->resetButton, SIGNAL(pressed()), this, SLOT(ResetViews()));
    connect(this->ui->AddDistance1Button, SIGNAL(pressed()), this, SLOT(AddDistanceMeasurementToView1()));
    // renderWindow->PrintSelf(std::cout, vtkIndent(14));
};

void QtVTKRenderWindows::slotExit() { qApp->exit(); }

#include "vtkImageActor.h"
void QtVTKRenderWindows::resliceMode(int mode) {
    this->ui->thickModeCheckBox->setEnabled(mode ? 1 : 0);
    this->ui->blendModeGroupBox->setEnabled(mode ? 1 : 0);

    for (int i = 0; i < 3; i++) {
        m_riv[i]->SetResliceMode(mode ? 1 : 0);
        m_riv[i]->GetRenderer()->ResetCamera();
        m_riv[i]->Render();
    }
}

void QtVTKRenderWindows::thickMode(int mode) {
    for (int i = 0; i < 3; i++) {
        m_riv[i]->SetThickMode(mode ? 1 : 0);
        double n[3] = {0};
        m_riv[i]->GetResliceCursor()->GetThickness(n);
        std::cout << i << ">thickness=" << n[0] << "," << n[1] << "," << n[2] << "\n";
        if (mode) {
            m_riv[i]->GetResliceCursor()->SetThickness(2, 3, 20);
            m_riv[i]->GetResliceCursor()->SetHole(10);
            vtkRenderer *ren = m_riv[i]->GetRenderer();
            vtkCamera *cam = ren->GetActiveCamera();
            cam->SetFocalPoint(0, 0, 0);
            double camPos[3] = {0, 0, 0};
            cam->Roll(45.f);
            camPos[i] = -1;
            //            cam->SetPosition(camPos);
            cam->SetParallelProjection(0);
            cam->SetViewUp(camPos);
            ren->ResetCamera();
            m_riv[i]->GetResliceCursorWidget()->SetEnabled(1);
        }
        //
        m_riv[i]->Render();
    }
}

void QtVTKRenderWindows::SetBlendMode(int m) {
    for (int i = 0; i < 3; i++) {
        vtkImageSlabReslice *thickSlabReslice =
            vtkImageSlabReslice::SafeDownCast(vtkResliceCursorThickLineRepresentation::SafeDownCast(
                                                  m_riv[i]->GetResliceCursorWidget()->GetRepresentation())
                                                  ->GetReslice());
        thickSlabReslice->SetBlendMode(m);
        m_riv[i]->Render();
    }
}

void QtVTKRenderWindows::SetBlendModeToMaxIP() { this->SetBlendMode(VTK_IMAGE_SLAB_MAX); }

void QtVTKRenderWindows::SetBlendModeToMinIP() { this->SetBlendMode(VTK_IMAGE_SLAB_MIN); }

void QtVTKRenderWindows::SetBlendModeToMeanIP() { this->SetBlendMode(VTK_IMAGE_SLAB_MEAN); }

void QtVTKRenderWindows::ResetViews() {
    // Reset the reslice image views
    for (int i = 0; i < 3; i++) {
        m_riv[i]->Reset();
        m_riv[i]->SetSliceOrientationToXY();
    }

    // Also sync the Image plane widget on the 3D top right view with any
    // changes to the reslice cursor.
    for (int i = 0; i < 3; i++) {
        vtkPlaneSource *ps = static_cast<vtkPlaneSource *>(planeWidget[i]->GetPolyDataAlgorithm());
        ps->SetNormal(m_riv[0]->GetResliceCursor()->GetPlane(i)->GetNormal());
        ps->SetCenter(m_riv[0]->GetResliceCursor()->GetPlane(i)->GetOrigin());

        // If the reslice plane has modified, update it on the 3D widget
        this->planeWidget[i]->UpdatePlacement();
    }

    // Render in response to changes.
    this->Render();
}

void QtVTKRenderWindows::Render() {
    for (int i = 0; i < 3; i++) {

        m_riv[i]->Render();
    }
    this->ui->view3->renderWindow()->Render();
}

void QtVTKRenderWindows::AddDistanceMeasurementToView1() {
    for (int i = 0; i < 3; i++) {
        AddDistanceMeasurementToView(i);
    }
    AddDistanceMeasurementToMPRView(3);
    // this->AddDistanceMeasurementToView(1);
    // AddFixedDistanceMeasurementToView(0);
}

void QtVTKRenderWindows::AddDistanceMeasurementToView(int i) {
    // remove existing widgets.
    if (this->DistanceWidget[i]) {
        std::cout << "DistanceWidget printSelf \033[36m i=" << i;
        // this->DistanceWidget[i]->GetDistanceRepresentation()->PrintSelf(std::cout, vtkIndent(3));

        std::cout << "DistanceWidget printSelf \033[0m\n\n" << std::endl;
        this->DistanceWidget[i]->SetEnabled(0);
        this->DistanceWidget[i] = nullptr;
    }

    // add new widget
    this->DistanceWidget[i] = vtkSmartPointer<vtkDistanceWidget>::New();
    this->DistanceWidget[i]->SetInteractor(this->m_riv[i]->GetResliceCursorWidget()->GetInteractor());

    // Set a priority higher than our reslice cursor widget
    this->DistanceWidget[i]->SetPriority(this->m_riv[i]->GetResliceCursorWidget()->GetPriority() + 0.01);

    // vtkSmartPointer<vtkPointHandleRepresentation2D> handleRep =
    // vtkSmartPointer<vtkPointHandleRepresentation2D>::New();
    // vtkSmartPointer<vtkDistanceRepresentation2D> distanceRep = vtkSmartPointer<vtkDistanceRepresentation2D>::New();
    // distanceRep->SetHandleRepresentation(handleRep);
    // this->DistanceWidget[i]->SetRepresentation(distanceRep);
    // distanceRep->InstantiateHandleRepresentation();
    // distanceRep->GetPoint1Representation()->SetPointPlacer(m_riv[i]->GetPointPlacer());
    // distanceRep->GetPoint2Representation()->SetPointPlacer(m_riv[i]->GetPointPlacer());

    // Add the distance to the list of widgets whose visibility is managed based
    // on the reslice plane by the ResliceImageViewerMeasurements class
    // this->m_riv[i]->GetMeasurements()->AddItem(this->DistanceWidget[i]);

    this->DistanceWidget[i]->CreateDefaultRepresentation();
    this->DistanceWidget[i]->EnabledOn();
}

void QtVTKRenderWindows::AddDistanceMeasurementToMPRView(int i) {
    // remove existing widgets.
    i = 3;
    if (this->DistanceWidget[i]) {
        // this->DistanceWidget[i]->GetDistanceRepresentation()->PrintSelf(std::cout, vtkIndent(3));
        this->DistanceWidget[i]->SetEnabled(0);
        this->DistanceWidget[i] = nullptr;
    }

    // add new widget
    this->DistanceWidget[i] = vtkSmartPointer<vtkDistanceWidget>::New();
    this->DistanceWidget[i]->SetInteractor(this->planeWidget[i - 3]->GetInteractor());

    // Set a priority higher than our reslice cursor widget
    this->DistanceWidget[i]->SetPriority(this->planeWidget[i - 3]->GetPriority() + 0.01);

    this->DistanceWidget[i]->CreateDefaultRepresentation();
    this->DistanceWidget[i]->EnabledOn();
    vtkSmartPointer<vtkDistanceWidget> dw = DistanceWidget[i];
    double pt[3] = {300, 200, 0};
    dw->GetDistanceRepresentation()->SetPoint1DisplayPosition(pt);
    pt[1] = 2;
    dw->GetDistanceRepresentation()->SetPoint2DisplayPosition(pt);
    dw->GetRepresentation()->VisibilityOn();
}

void QtVTKRenderWindows::AddFixedDistanceMeasurementToView(int i) {
    vtkRenderer *ren = this->m_riv[i]->GetRenderer();
    // remove existing widgets.
    if (this->DistanceWidget[i]) {
        this->DistanceWidget[i]->SetEnabled(0);
        this->DistanceWidget[i] = nullptr;
    }

    vtkSmartPointer<vtkDistanceRepresentation2D> distanceRep = vtkSmartPointer<vtkDistanceRepresentation2D>::New();
    distanceRep->InstantiateHandleRepresentation();
    distanceRep->SetRenderer(ren);
    double pos[3] = {0, 0, 0}, pos2[3] = {0.4, 0.5, 0};
    distanceRep->GetPoint1Representation()->SetDisplayPosition(pos);
    distanceRep->GetPoint2Representation()->SetDisplayPosition(pos2);
    distanceRep->BuildRepresentation();
    // distanceRep->

    // Add the distance to the list of widgets whose visibility is managed based
    // on the reslice plane by the ResliceImageViewerMeasurements class
    std::cout << "create distance rep.";
}

int QtVTKRenderWindows::addScale() {
    vtkResliceImageViewer *ipw = m_riv[0];
    vtkRenderer *renderer = ipw->GetRenderer();
    vtkNew<vtkLegendScaleActor> legendScaleActor;
    // legendScaleActor->AllAnnotationsOff(); //this will show nothing.

    legendScaleActor->SetLabelModeToDistance();
    legendScaleActor->LegendVisibilityOff(); // this will remove bottom scale only keep 4 axis
    legendScaleActor->SetLeftAxisVisibility(0);
    legendScaleActor->SetTopAxisVisibility(0);
    legendScaleActor->SetBottomAxisVisibility(0);
    vtkAxisActor2D *axis = legendScaleActor->GetRightAxis();
    axis->SetFontFactor(1.2);
    axis->SetLabelFormat("%.0fmm");
    axis->SetTickOffset(0);
    axis->SetNumberOfLabels(3);
    axis->PrintSelf(std::cout, vtkIndent(4));

    // Add the actor to the scene
    renderer->AddActor(legendScaleActor);
}

int QtVTKRenderWindows::addDistanceScaleV4(const int sliceViewIdx) {
    vtkSmartPointer<vtkAxisActor2D> distanceScale = vtkSmartPointer<vtkAxisActor2D>::New();
    m_distanceScale = distanceScale;
    distanceScale->GetPositionCoordinate()->SetCoordinateSystemToViewport();
    distanceScale->GetPosition2Coordinate()->SetCoordinateSystemToViewport();
    distanceScale->GetPositionCoordinate()->SetReferenceCoordinate(nullptr);
    distanceScale->AdjustLabelsOff();

    vtkResliceImageViewer *ipw = m_riv[sliceViewIdx];
    vtkRenderer *ren = ipw->GetRenderer();
    const int *size = ren->GetSize();

    double RightBorderOffset = 50, CornerOffsetFactor = 2.0, BottomBorderOffset = 30;

    distanceScale->GetPositionCoordinate()->SetValue(size[0] - RightBorderOffset,
                                                     BottomBorderOffset * CornerOffsetFactor, 0.0);
    distanceScale->GetPosition2Coordinate()->SetValue(size[0] - RightBorderOffset,
                                                      size[1] - CornerOffsetFactor * BottomBorderOffset, 0.0);
    double *xL = distanceScale->GetPositionCoordinate()->GetComputedWorldValue(ren);
    double *xR = distanceScale->GetPosition2Coordinate()->GetComputedWorldValue(ren);
    bool usingDistance = 1;
    if (!usingDistance) {
        distanceScale->SetRange(xL[1], xR[1]);
    } else {
        double d = sqrt(vtkMath::Distance2BetweenPoints(xL, xR));
        distanceScale->SetRange(-d / 2.0, d / 2.0);
    }

    std::cout << "distanceScale printSelf \033[36m";
    distanceScale->PrintSelf(std::cout, vtkIndent(4));
    std::cout << "distanceScale printSelf \033[0m" << std::endl;
    ren->AddActor(distanceScale);
    return 0;
}

int QtVTKRenderWindows::addDistanceScale(const int sliceViewIdx) {
    const int orientation = 0;
    vtkSmartPointer<vtkPointPlacer> pointplacer = vtkSmartPointer<vtkPointPlacer>::New();
    vtkSmartPointer<vtkCoordinate> coords = vtkSmartPointer<vtkCoordinate>::New();
    // vtkSmartPointer<vtkAxisActor> distanceScale = vtkSmartPointer<vtkAxisActor>::New();
    vtkSmartPointer<vtkAxisActor2D> distanceScale = vtkSmartPointer<vtkAxisActor2D>::New();
    m_distanceScale = distanceScale;
    distanceScale->GetPositionCoordinate()->SetCoordinateSystemToViewport();
    distanceScale->GetPosition2Coordinate()->SetCoordinateSystemToViewport();
    distanceScale->GetPositionCoordinate()->SetReferenceCoordinate(nullptr);

    int extents[6];
    double spacings[3] = {1};
    m_data->GetExtent(extents);
    m_data->GetSpacing(spacings);

    // ren->PrintSelf(std::cerr, vtkIndent(4));

    std::cout << ("add-DistanceScale\033[32m ============================================\033[0mBegin\n");

    float Xmax = (extents[1] - extents[0]) * spacings[0] - 2;
    float Ymax = (extents[3] - extents[2]) * spacings[1] - 2;
    float Zmax = (extents[5] - extents[4]) * spacings[2] - 2;

    vtkResliceImageViewer *ipw = m_riv[sliceViewIdx];
    double normal[9] = {0};

    vtkRenderer *ren = ipw->GetRenderer();
    const int *size = ren->GetSize();
    distanceScale->GetPositionCoordinate()->SetValue(size[0], 0.0, 0.0);
    distanceScale->GetPosition2Coordinate()->SetValue(size[0], size[1], 0.0);
    double *xL = distanceScale->GetPositionCoordinate()->GetComputedWorldValue(ren);
    double *xR = distanceScale->GetPosition2Coordinate()->GetComputedWorldValue(ren);
    bool usingDistance = 0;
    if (!usingDistance) {
        distanceScale->SetRange(xL[1], xR[1]);
    } else {
        double d = sqrt(vtkMath::Distance2BetweenPoints(xL, xR));
        distanceScale->SetRange(-d / 2.0, d / 2.0);
    }
    if (nullptr == ipw) {
        return -1;
    }
    int imgSize[2];
    int *sizePtr = ipw->GetSize();
    imgSize[0] = *sizePtr;
    imgSize[1] = *(sizePtr + 1);
    std::cout << "imgSize=" << imgSize[0] << "," << imgSize[1] << std::endl;

    const double distancePositonWorldCoords = 0.98 * imgSize[0];
    for (int orent = 0; orent < 1; orent++) {
        int extIdx = 2 * orent;
        float center = (extents[extIdx + 1] - extents[extIdx]) * spacings[orent];
        float start = center - 5.f;
        float end = center + 5.f;
        //        if (0 == orent) {
        //            // A plane
        double p1[3] = {start, Ymax * 1.2, Zmax};
        double p2[3] = {end, Ymax * 1.2, Zmax};
        //            // distanceScale->GetPoint1Coordinate()->SetValue(p1);
        //            distanceScale->SetPoint1(p1);
        //            distanceScale->SetPoint2(p2);
        //            // distanceScale->GetPoint2Coordinate()->SetValue(p2);
        //        }
        p1[0] = distancePositonWorldCoords, p1[1] = 0.7 * imgSize[1];
        p2[0] = distancePositonWorldCoords, p2[1] = 0.3 * imgSize[1];
        double Distance = sqrt(vtkMath::Distance2BetweenPoints(p1, p2)); // it is not mapping to render.
#if 0
        double worldPos[2][3];
        ipw->GetPointPlacer()->ComputeWorldPosition(ren, p1, worldPos[0], normal);
        ipw->GetPointPlacer()->ComputeWorldPosition(ren, p2, worldPos[1], normal);

        distanceScale->GetPoint1Coordinate()->SetCoordinateSystemToDisplay();
        distanceScale->GetPoint2Coordinate()->SetCoordinateSystemToDisplay();
        distanceScale->GetPoint1Coordinate()->SetValue(worldPos[0]);
        distanceScale->GetPoint2Coordinate()->SetValue(worldPos[1]);
        /*
        if (0 == sliceViewIdx) {
            distanceScale->SetPosition(worldPos[0][1], worldPos[0][2]);
            distanceScale->SetPoint2(worldPos[1][1], worldPos[1][2]);
        }
        if (1 == sliceViewIdx) {
            distanceScale->SetPosition(worldPos[0][1], worldPos[0][0]);
            distanceScale->SetPoint2(worldPos[1][1], worldPos[1][0]);
        }
        if (2 == sliceViewIdx) {
            distanceScale->SetPosition(worldPos[0][1], worldPos[0][0]);
            distanceScale->SetPoint2(worldPos[1][1], worldPos[1][0]);
        }*/
#else
        distanceScale->SetPoint1(0.98, 0.7);
        distanceScale->SetPoint2(0.98, 0.4);
#endif
        distanceScale->SetRulerMode(1);
        distanceScale->AdjustLabelsOn();
        // distanceScale->SetRulerDistance(1.0);
        distanceScale->LabelVisibilityOff();
        distanceScale->SetFontFactor(1.5);
        // vtkTextProperty *title = distanceScale->GetTitleTextProperty();
        // title->SetFontSize(16);
        char string[512] = {0};
        snprintf(string, 512, "%-#6.3g", Distance);

        distanceScale->SetTickLength(5);
        distanceScale->SetTitle(string);
        ren->AddActor(distanceScale);
    }
    // distanceScale->PrintSelf(std::cout, vtkIndent(4));
    std::cout << ("add-DistanceScale\033[32m ============================================\033[0mEnd\n");
    return 0;
}

int QtVTKRenderWindows::addDistanceScaleV3(const int sliceViewIdx) {
    vtkResliceImageViewer *ipw = m_riv[sliceViewIdx];
    vtkRenderWindowInteractor *iren = ipw->GetInteractor();
    vtkRenderer *ren = ipw->GetRenderer();
    vtkRenderer *ren1 = ren;
    vtkRenderWindow *renWin = ren->GetRenderWindow();
    return 0;
}

/*
=========================================
==========================================
*/

// This callback is responsible for adjusting the point position.
// It looks in the region around the point and finds the maximum or
// minimum value.
class vtkDistanceCallback : public vtkCommand {
  public:
    static vtkDistanceCallback *New() { return new vtkDistanceCallback; }
    void Execute(vtkObject *caller, unsigned long, void *) override;
    vtkDistanceCallback() : Renderer(nullptr), RenderWindow(nullptr), DistanceWidget(nullptr), Distance(nullptr) {}
    vtkRenderer *Renderer;
    vtkRenderWindow *RenderWindow;
    vtkDistanceWidget *DistanceWidget;
    vtkDistanceRepresentation2D *Distance;
};

// Method re-positions the points using random perturbation
void vtkDistanceCallback::Execute(vtkObject *, unsigned long eid, void *callData) {
    if (eid == vtkCommand::InteractionEvent || eid == vtkCommand::EndInteractionEvent) {
        double pos1[3], pos2[3];
        // Modify the measure axis
        this->Distance->GetPoint1WorldPosition(pos1);
        this->Distance->GetPoint2WorldPosition(pos2);
        double dist = sqrt(vtkMath::Distance2BetweenPoints(pos1, pos2));

        char title[256];
        this->Distance->GetAxis()->SetRange(0.0, dist);
        snprintf(title, sizeof(title), "%-#6.3g", dist);
        this->Distance->GetAxis()->SetTitle(title);
    } else {
        int pid = *(reinterpret_cast<int *>(callData));

        // From the point id, get the display coordinates
        double pos1[3], pos2[3], *pos;
        this->Distance->GetPoint1DisplayPosition(pos1);
        this->Distance->GetPoint2DisplayPosition(pos2);
        if (pid == 0) {
            pos = pos1;
        } else {
            pos = pos2;
        }

        // Okay, render without the widget, and get the color buffer
        int enabled = this->DistanceWidget->GetEnabled();
        if (enabled) {
            this->DistanceWidget->SetEnabled(0); // does a Render() as a side effect
        }

        // Pretend we are doing something serious....just randomly bump the
        // location of the point.
        double p[3];
        p[0] = pos[0] + static_cast<int>(vtkMath::Random(-5.5, 5.5));
        p[1] = pos[1] + static_cast<int>(vtkMath::Random(-5.5, 5.5));
        p[2] = 0.0;

        // Set the new position
        if (pid == 0) {
            this->Distance->SetPoint1DisplayPosition(p);
        } else {
            this->Distance->SetPoint2DisplayPosition(p);
        }

        // Side effect of a render here
        if (enabled) {
            this->DistanceWidget->SetEnabled(1);
        }
    }
}

const char TestDistanceWidgetEventLog[] = "# StreamVersion 1\n"
                                          "RenderEvent 0 0 0 0 0 0 0\n"
                                          "EnterEvent 292 123 0 0 0 0 0\n"
                                          "MouseMoveEvent 280 131 0 0 0 0 0\n"
                                          "MouseMoveEvent 268 137 0 0 0 0 0\n"
                                          "MouseMoveEvent 258 143 0 0 0 0 0\n"
                                          "MouseMoveEvent 250 147 0 0 0 0 0\n"
                                          "MouseMoveEvent 246 153 0 0 0 0 0\n"
                                          "MouseMoveEvent 245 155 0 0 0 0 0\n"
                                          "MouseMoveEvent 244 157 0 0 0 0 0\n"
                                          "MouseMoveEvent 240 161 0 0 0 0 0\n"
                                          "MouseMoveEvent 239 163 0 0 0 0 0\n"
                                          "MouseMoveEvent 235 167 0 0 0 0 0\n"
                                          "MouseMoveEvent 233 173 0 0 0 0 0\n"
                                          "MouseMoveEvent 229 177 0 0 0 0 0\n"
                                          "MouseMoveEvent 223 183 0 0 0 0 0\n"
                                          "MouseMoveEvent 222 184 0 0 0 0 0\n"
                                          "MouseMoveEvent 221 186 0 0 0 0 0\n"
                                          "MouseMoveEvent 220 187 0 0 0 0 0\n"
                                          "MouseMoveEvent 219 188 0 0 0 0 0\n"
                                          "MouseMoveEvent 219 189 0 0 0 0 0\n"
                                          "LeftButtonPressEvent 219 189 0 0 0 0 0\n"
                                          "RenderEvent 219 189 0 0 0 0 0\n"
                                          "LeftButtonReleaseEvent 219 189 0 0 0 0 0\n"
                                          "MouseMoveEvent 218 189 0 0 0 0 0\n"
                                          "RenderEvent 218 189 0 0 0 0 0\n"
                                          "RenderEvent 200 48 0 0 0 0 0\n"
                                          "KeyPressEvent 222 88 0 0 113 1 q\n"
                                          "CharEvent 222 88 0 0 113 1 q\n"
                                          "ExitEvent 222 88 0 0 113 1 q\n";
#define VTK_CREATE(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int QtVTKRenderWindows::addDistanceScaleV2(const int sliceViewIdx) {
    vtkResliceImageViewer *ipw = m_riv[sliceViewIdx];
    vtkRenderWindowInteractor *iren = ipw->GetInteractor();
    vtkRenderer *ren = ipw->GetRenderer();
    vtkRenderer *ren1 = ren;
    vtkRenderWindow *renWin = ren->GetRenderWindow();

    // Create the widget and its representation
    VTK_CREATE(vtkPointHandleRepresentation2D, handle);
    handle->GetProperty()->SetColor(1, 0, 0);
    VTK_CREATE(vtkDistanceRepresentation2D, rep);
    rep->SetHandleRepresentation(handle);
    vtkAxisActor2D *axis = rep->GetAxis();
    axis->UseFontSizeFromPropertyOn();
    vtkTextProperty *titleProp = axis->GetTitleTextProperty();
    titleProp->SetFontSize(40);
    if (!axis) {
        std::cerr << "Error getting representation's axis" << std::endl;
        return EXIT_FAILURE;
    }
    axis->SetNumberOfMinorTicks(4);
    axis->SetTickLength(9);
    axis->SetTitlePosition(0.2);
    rep->RulerModeOn();
    rep->SetRulerDistance(0.25);
    if (rep->GetRulerDistance() != 0.25) {
        std::cerr << "Error setting ruler distance to 0.25, get returned " << rep->GetRulerDistance() << std::endl;
        return EXIT_FAILURE;
    }

    vtkProperty2D *prop2D = rep->GetAxisProperty();
    if (!prop2D) {
        std::cerr << "Error getting widget axis property" << std::endl;
        return EXIT_FAILURE;
    }
    prop2D->SetColor(1.0, 0.0, 1.0);

    VTK_CREATE(vtkDistanceWidget, widget);
    widget->SetInteractor(iren);
    widget->CreateDefaultRepresentation();
    widget->SetRepresentation(rep);

    VTK_CREATE(vtkDistanceCallback, mcbk);
    mcbk->Renderer = ren1;
    mcbk->RenderWindow = renWin;
    mcbk->Distance = rep;
    mcbk->DistanceWidget = widget;

    // record events
    VTK_CREATE(vtkInteractorEventRecorder, recorder);
    recorder->SetInteractor(iren);
    recorder->On();
    // recorder->SetFileName("/tmp/record2.log");
    // recorder->Record();
    recorder->ReadFromInputStringOn();
    recorder->SetInputString(TestDistanceWidgetEventLog);
    widget->SetPriority(this->m_riv[0]->GetResliceCursorWidget()->GetPriority() + 0.01);
    widget->On();

    recorder->Play();

    return 0;
}
