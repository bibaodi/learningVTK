
#include "QtVTKRenderWindows.h"
#include "ui_QtVTKRenderWindows.h"

#include "vtkBoundedPlanePointPlacer.h"
#include "vtkCellPicker.h"
#include "vtkCommand.h"
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
            vtkResliceCursor *rslc = RCW[0]->GetResliceCursorRepresentation()->GetResliceCursor();
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
                    for (int i = 0; i < 3; i++) {
                        vtkPlaneSource *ps = static_cast<vtkPlaneSource *>(this->IPW[i]->GetPolyDataAlgorithm());
                        ps->SetOrigin(this->RCW[i]->GetResliceCursorRepresentation()->GetPlaneSource()->GetOrigin());
                        ps->SetPoint1(this->RCW[i]->GetResliceCursorRepresentation()->GetPlaneSource()->GetPoint1());
                        ps->SetPoint2(this->RCW[i]->GetResliceCursorRepresentation()->GetPlaneSource()->GetPoint2());

                        // If the reslice plane has modified, update it on the 3D widget
                        this->IPW[i]->UpdatePlacement();
                    }
                }
                // psource->PrintSelf(std::cout, vtkIndent(4));
            }
        }

        // Render everything
        for (int i = 0; i < 3; i++) {
            // eton this->RCW[i]->Set
            this->RCW[i]->Render();
            this->IPW[i]->UpdatePlacement();
        }
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
    // clang-format off
    // clang-format on
    QVTKRenderWidget *views[3];
    views[0] = this->ui->view1;
    views[1] = this->ui->view2;
    views[2] = this->ui->view3;
    for (int i = 0; i < 3; i++) {
        riw[i] = vtkSmartPointer<vtkResliceImageViewer>::New();
        std::cout << "vtkResliceImageViewer[" << i << "] Pointer=" << riw[i] << "\n";
        vtkNew<vtkGenericOpenGLRenderWindow> renderWindow;
        riw[i]->SetRenderWindow(renderWindow);

        views[i]->setRenderWindow(riw[i]->GetRenderWindow());
        QPoint posLocal = views[i]->pos();
        int posX = views[i]->mapToGlobal(posLocal).x();
        int posY = views[i]->mapToGlobal(posLocal).y();
        std::cout << "View position[" << i << "]=" << posX << "," << posY << ")\n";
        riw[i]->SetupInteractor(views[i]->renderWindow()->GetInteractor());
        double displaypt[3];
        riw[i]->GetRenderer()->GetDisplayPoint(displaypt);
        // renderWindow->PrintSelf(std::cout, vtkIndent(4));
    }

    //    this->ui->view2->setRenderWindow(riw[1]->GetRenderWindow());
    //    riw[1]->SetupInteractor(this->ui->view2->renderWindow()->GetInteractor());

    //    this->ui->view3->setRenderWindow(riw[2]->GetRenderWindow());
    //    riw[2]->SetupInteractor(this->ui->view3->renderWindow()->GetInteractor());

    vtkResliceCursor *rslc = riw[1]->GetResliceCursor();
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
    rslc->PrintSelf(std::cout, vtkIndent(4));

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
            vtkResliceCursorLineRepresentation::SafeDownCast(riw[i]->GetResliceCursorWidget()->GetRepresentation());
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
        // riw[i]->SetResliceCursor(riw[0]->GetResliceCursor());
        riw[i]->SetResliceCursor(rslc);

        riw[i]->SetInputData(reader->GetOutput());
        riw[i]->SetSliceOrientation(i);
        riw[i]->SetResliceModeToAxisAligned();

        /*This will change Image size on Screen, default if Parallel projection. shoud
         not change to close.*/
        /**I want to correct slice direction --begin**/
        vtkRenderer *ren = riw[i]->GetRenderer();
        ren->ResetCamera();

        riw[i]->SetResliceModeToOblique();

        vtkCamera *cam = ren->GetActiveCamera();
        cam->SetObliqueAngles(30.f, 60.345);
        ren->ResetCamera();
        std::cout << "camera printSelf \033[36m";
        // cam->PrintSelf(std::cout, vtkIndent(4));
        std::cout << "camera printSelf \033[0m" << std::endl;

        /**I want to correct slice direction --begin.end**/

        riw[i]->GetResliceCursorWidget()->SetEnabled(1);
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
        riw[i]->GetRenderer()->SetBackground(color);

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

    vtkSmartPointer<vtkResliceCursorCallback> cbk = vtkSmartPointer<vtkResliceCursorCallback>::New();

    for (int i = 0; i < 3; i++) {
        cbk->IPW[i] = planeWidget[i];
        cbk->RCW[i] = riw[i]->GetResliceCursorWidget();
        riw[i]->GetResliceCursorWidget()->AddObserver(vtkResliceCursorWidget::ResliceAxesChangedEvent, cbk);
        riw[i]->GetResliceCursorWidget()->AddObserver(vtkResliceCursorWidget::WindowLevelEvent, cbk);
        riw[i]->GetResliceCursorWidget()->AddObserver(vtkResliceCursorWidget::ResliceThicknessChangedEvent, cbk);
        riw[i]->GetResliceCursorWidget()->AddObserver(vtkResliceCursorWidget::ResetCursorEvent, cbk);
        riw[i]->GetInteractorStyle()->AddObserver(vtkCommand::WindowLevelEvent, cbk);
        riw[i]->AddObserver(vtkResliceImageViewer::SliceChangedEvent, cbk);
        // planeWidget[i]->AddObserver(vtkCommand::AnyEvent, cbk);

        // Make them all share the same color map.
        riw[i]->SetLookupTable(riw[0]->GetLookupTable());
        planeWidget[i]->GetColorMap()->SetLookupTable(riw[0]->GetLookupTable());
        // planeWidget[i]->GetColorMap()->SetInput(riw[i]->GetResliceCursorWidget()->GetResliceCursorRepresentation()->GetColorMap()->GetInput());
        planeWidget[i]->SetColorMap(riw[i]->GetResliceCursorWidget()->GetResliceCursorRepresentation()->GetColorMap());
        // sync?
        /*
                vtkPlaneSource *ps = static_cast<vtkPlaneSource *>(planeWidget[i]->GetPolyDataAlgorithm());
                ps->SetNormal(riw[0]->GetResliceCursor()->GetPlane(i)->GetNormal());
                ps->SetCenter(riw[0]->GetResliceCursor()->GetPlane(i)->GetOrigin());
          */
    }

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
        riw[i]->SetResliceMode(mode ? 1 : 0);
        //        riw[i]->GetImageActor()->RotateX(45); for not using ImageActor, so this setting not running.
        riw[i]->GetRenderer()->ResetCamera();
        riw[i]->Render();
    }
}

void QtVTKRenderWindows::thickMode(int mode) {
    for (int i = 0; i < 3; i++) {
        riw[i]->SetThickMode(mode ? 1 : 0);
        double n[3] = {0};
        riw[i]->GetResliceCursor()->GetThickness(n);
        std::cout << i << ">thickness=" << n[0] << "," << n[1] << "," << n[2] << "\n";
        if (mode) {
            riw[i]->GetResliceCursor()->SetThickness(2, 3, 20);
            riw[i]->GetResliceCursor()->SetHole(10);
            vtkRenderer *ren = riw[i]->GetRenderer();
            vtkCamera *cam = ren->GetActiveCamera();
            cam->SetFocalPoint(0, 0, 0);
            double camPos[3] = {0, 0, 0};
            cam->Roll(45.f);
            camPos[i] = -1;
            //            cam->SetPosition(camPos);
            cam->SetParallelProjection(0);
            cam->SetViewUp(camPos);
            ren->ResetCamera();
            riw[i]->GetResliceCursorWidget()->SetEnabled(1);
        }
        //
        riw[i]->Render();
    }
}

void QtVTKRenderWindows::SetBlendMode(int m) {
    for (int i = 0; i < 3; i++) {
        vtkImageSlabReslice *thickSlabReslice = vtkImageSlabReslice::SafeDownCast(
            vtkResliceCursorThickLineRepresentation::SafeDownCast(riw[i]->GetResliceCursorWidget()->GetRepresentation())
                ->GetReslice());
        thickSlabReslice->SetBlendMode(m);
        riw[i]->Render();
    }
}

void QtVTKRenderWindows::SetBlendModeToMaxIP() { this->SetBlendMode(VTK_IMAGE_SLAB_MAX); }

void QtVTKRenderWindows::SetBlendModeToMinIP() { this->SetBlendMode(VTK_IMAGE_SLAB_MIN); }

void QtVTKRenderWindows::SetBlendModeToMeanIP() { this->SetBlendMode(VTK_IMAGE_SLAB_MEAN); }

void QtVTKRenderWindows::ResetViews() {
    // Reset the reslice image views
    for (int i = 0; i < 3; i++) {
        riw[i]->Reset();
        riw[i]->SetSliceOrientationToXY();
    }

    // Also sync the Image plane widget on the 3D top right view with any
    // changes to the reslice cursor.
    for (int i = 0; i < 3; i++) {
        vtkPlaneSource *ps = static_cast<vtkPlaneSource *>(planeWidget[i]->GetPolyDataAlgorithm());
        ps->SetNormal(riw[0]->GetResliceCursor()->GetPlane(i)->GetNormal());
        ps->SetCenter(riw[0]->GetResliceCursor()->GetPlane(i)->GetOrigin());

        // If the reslice plane has modified, update it on the 3D widget
        this->planeWidget[i]->UpdatePlacement();
    }

    // Render in response to changes.
    this->Render();
}

void QtVTKRenderWindows::Render() {
    for (int i = 0; i < 3; i++) {

        riw[i]->Render();
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
        this->DistanceWidget[i]->GetDistanceRepresentation()->PrintSelf(std::cout, vtkIndent(3));
        this->DistanceWidget[i]->SetEnabled(0);
        this->DistanceWidget[i] = nullptr;
    }

    // add new widget
    this->DistanceWidget[i] = vtkSmartPointer<vtkDistanceWidget>::New();
    this->DistanceWidget[i]->SetInteractor(this->riw[i]->GetResliceCursorWidget()->GetInteractor());

    // Set a priority higher than our reslice cursor widget
    this->DistanceWidget[i]->SetPriority(this->riw[i]->GetResliceCursorWidget()->GetPriority() + 0.01);

    // vtkSmartPointer<vtkPointHandleRepresentation2D> handleRep =
    // vtkSmartPointer<vtkPointHandleRepresentation2D>::New();
    // vtkSmartPointer<vtkDistanceRepresentation2D> distanceRep = vtkSmartPointer<vtkDistanceRepresentation2D>::New();
    // distanceRep->SetHandleRepresentation(handleRep);
    // this->DistanceWidget[i]->SetRepresentation(distanceRep);
    // distanceRep->InstantiateHandleRepresentation();
    // distanceRep->GetPoint1Representation()->SetPointPlacer(riw[i]->GetPointPlacer());
    // distanceRep->GetPoint2Representation()->SetPointPlacer(riw[i]->GetPointPlacer());

    // Add the distance to the list of widgets whose visibility is managed based
    // on the reslice plane by the ResliceImageViewerMeasurements class
    // this->riw[i]->GetMeasurements()->AddItem(this->DistanceWidget[i]);

    this->DistanceWidget[i]->CreateDefaultRepresentation();
    this->DistanceWidget[i]->EnabledOn();
}

void QtVTKRenderWindows::AddDistanceMeasurementToMPRView(int i) {
    // remove existing widgets.
    i = 3;
    if (this->DistanceWidget[i]) {
        this->DistanceWidget[i]->GetDistanceRepresentation()->PrintSelf(std::cout, vtkIndent(3));
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
}

void QtVTKRenderWindows::AddFixedDistanceMeasurementToView(int i) {
    vtkRenderer *ren = this->riw[i]->GetRenderer();
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
