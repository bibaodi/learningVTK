
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
            double *wl = static_cast<double *>(callData);
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
            std::cout << "Caller vtkResliceImageViewer="
                      << "\n";
            vtkResliceCursor *rslc = RCW[0]->GetResliceCursorRepresentation()->GetResliceCursor();
            for (int i = 0; i < 3; i++) {
                vtkPlaneSource *psource = RCW[i]->GetResliceCursorRepresentation()->GetPlaneSource();
                double planeOrigin[3];
                psource->GetOrigin(planeOrigin);
                std::cout << "planeOrigin[" << i << "]=" << planeOrigin[0] << ",\t" << planeOrigin[1] << ", \t"
                          << planeOrigin[2];
                std::cout << std::endl;
                IPW[i]->SetSlicePosition(planeOrigin[i]);
                // rotate it.
                double axies[3] = {0, 0, 1};
                psource->Rotate(90, axies);
                psource->PrintSelf(std::cout, vtkIndent(4));
            }
            vtkResliceCursorPolyDataAlgorithm *rslcAlgo =
                RCW[0]->GetResliceCursorRepresentation()->GetCursorAlgorithm();
            rslcAlgo->UpdateInformation();
            vtkInformation *outInfo = rslcAlgo->GetOutputInformation(0);
            double origin[3];
            outInfo->Get(vtkDataObject::ORIGIN(), origin);
            double spacing[3];
            outInfo->Get(vtkDataObject::SPACING(), spacing);
            for (int i = 0; i < 3; i++) {
                // rslc->PrintSelf(std::cout, vtkIndent(4));
                double sliceBounds[6] = {110};
                rslcAlgo->GetSliceBounds(sliceBounds);
                std::cout << "sliceBounds=" << sliceBounds[0] << "," << sliceBounds[1] << ", " << sliceBounds[2] << ", "
                          << sliceBounds[3] << ", " << sliceBounds[4] << ", " << sliceBounds[5] << std::endl;
            }
        }
        //        vtkImageViewer2 *imgv = dynamic_cast<vtkImageViewer2 *>(caller);
        //        if (imgv) {
        //            std::cout << "Caller vtkImageViewer2="
        //                      << "\n";
        //        }

        // Render everything
        for (int i = 0; i < 3; i++) {
            // eton this->RCW[i]->Set
            this->RCW[i]->Render();
        }
        this->IPW[0]->GetInteractor()->GetRenderWindow()->Render();
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
        vtkNew<vtkGenericOpenGLRenderWindow> renderWindow;
        riw[i]->SetRenderWindow(renderWindow);

        views[i]->setRenderWindow(riw[i]->GetRenderWindow());
        riw[i]->SetupInteractor(views[i]->renderWindow()->GetInteractor());
    }

//    this->ui->view2->setRenderWindow(riw[1]->GetRenderWindow());
//    riw[1]->SetupInteractor(this->ui->view2->renderWindow()->GetInteractor());

//    this->ui->view3->setRenderWindow(riw[2]->GetRenderWindow());
//    riw[2]->SetupInteractor(this->ui->view3->renderWindow()->GetInteractor());
#undef VIEWNAME
#undef __VIEWNAME__
    vtkResliceCursor *rslc = riw[1]->GetResliceCursor();
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
        riw[i]->SetResliceModeToOblique();
        // vtkResliceCursorWidget *rslicw = riw[i]->GetResliceCursorWidget();
        rep->PrintSelf(std::cout, vtkIndent(2));
        // rslicw->GetResliceCursorRepresentation()->GetCursorAlgorithm()->GetInputAlgorithm();
        vtkRenderer *ren = riw[i]->GetRenderer();
        vtkCamera *cam = ren->GetActiveCamera();
        cam->SetObliqueAngles(30.f, 60.345);
        cam->SetFocalPoint(0, 0, 0);
        double camPos[3] = {0, 0, 0};
        camPos[i] = 1;
        cam->SetPosition(camPos);
        cam->Roll(70);
        cam->SetParallelProjection(0);
        ren->ResetCamera();
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
        double colorB[3] = {0.1, 0.2, 0.5};
        double color[3] = {0, 0, 0};
        color[i] = 1;
        planeWidget[i]->GetPlaneProperty()->SetColor(color);

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

void QtVTKRenderWindows::AddDistanceMeasurementToView1() { this->AddDistanceMeasurementToView(1); }

void QtVTKRenderWindows::AddDistanceMeasurementToView(int i) {
    // remove existing widgets.
    if (this->DistanceWidget[i]) {
        this->DistanceWidget[i]->SetEnabled(0);
        this->DistanceWidget[i] = nullptr;
    }

    // add new widget
    this->DistanceWidget[i] = vtkSmartPointer<vtkDistanceWidget>::New();
    this->DistanceWidget[i]->SetInteractor(this->riw[i]->GetResliceCursorWidget()->GetInteractor());

    // Set a priority higher than our reslice cursor widget
    this->DistanceWidget[i]->SetPriority(this->riw[i]->GetResliceCursorWidget()->GetPriority() + 0.01);

    vtkSmartPointer<vtkPointHandleRepresentation2D> handleRep = vtkSmartPointer<vtkPointHandleRepresentation2D>::New();
    vtkSmartPointer<vtkDistanceRepresentation2D> distanceRep = vtkSmartPointer<vtkDistanceRepresentation2D>::New();
    distanceRep->SetHandleRepresentation(handleRep);
    this->DistanceWidget[i]->SetRepresentation(distanceRep);
    distanceRep->InstantiateHandleRepresentation();
    distanceRep->GetPoint1Representation()->SetPointPlacer(riw[i]->GetPointPlacer());
    distanceRep->GetPoint2Representation()->SetPointPlacer(riw[i]->GetPointPlacer());

    // Add the distance to the list of widgets whose visibility is managed based
    // on the reslice plane by the ResliceImageViewerMeasurements class
    this->riw[i]->GetMeasurements()->AddItem(this->DistanceWidget[i]);

    this->DistanceWidget[i]->CreateDefaultRepresentation();
    this->DistanceWidget[i]->EnabledOn();
}

void QtVTKRenderWindows::AddFixedDistanceMeasurementToView(int i) {
    // remove existing widgets.
    if (this->DistanceWidget[i]) {
        this->DistanceWidget[i]->SetEnabled(0);
        this->DistanceWidget[i] = nullptr;
    }

    // add new widget
    this->DistanceWidget[i] = vtkSmartPointer<vtkDistanceWidget>::New();
    // this->DistanceWidget[i]->SetInteractor(this->riw[i]->GetResliceCursorWidget()->GetInteractor());
    this->DistanceWidget[i]->RemoveAllObservers();
    // Set a priority higher than our reslice cursor widget
    // this->DistanceWidget[i]->SetPriority(this->riw[i]->GetResliceCursorWidget()->GetPriority() + 0.01);

    vtkSmartPointer<vtkPointHandleRepresentation2D> handleRep = vtkSmartPointer<vtkPointHandleRepresentation2D>::New();
    vtkSmartPointer<vtkDistanceRepresentation2D> distanceRep = vtkSmartPointer<vtkDistanceRepresentation2D>::New();
    distanceRep->SetHandleRepresentation(handleRep);
    this->DistanceWidget[i]->SetRepresentation(distanceRep);
    distanceRep->InstantiateHandleRepresentation();
    double pos[3] = {0, 0, 0}, pos2[3] = {100, 100, 0};
    distanceRep->GetPoint1Representation()->SetDisplayPosition(pos);
    distanceRep->GetPoint2Representation()->SetDisplayPosition(pos2);

    // Add the distance to the list of widgets whose visibility is managed based
    // on the reslice plane by the ResliceImageViewerMeasurements class
    this->riw[i]->GetMeasurements()->AddItem(this->DistanceWidget[i]);

    this->DistanceWidget[i]->CreateDefaultRepresentation();
    this->DistanceWidget[i]->EnabledOn();
}
