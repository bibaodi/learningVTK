#include <vtkActor.h>
#include <vtkAxisActor.h>
#include <vtkCamera.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkStringArray.h>
#include <vtkTextProperty.h>

//----------------------------------------------------------------------------
int main(int, char *[]) {
    vtkNew<vtkNamedColors> colors;

    vtkNew<vtkSphereSource> source;
    source->SetPhiResolution(31);
    source->SetThetaResolution(31);
    source->Update();
    double bounds[6];
    source->GetOutput()->GetBounds(bounds);
    std::cout << "Bounds: " << bounds[0] << ", " << bounds[2] << ", " << bounds[3] << ", " << bounds[1] << ", "
              << bounds[3] << ", " << bounds[5] << std::endl;
    double center[3];
    source->GetOutput()->GetCenter(center);
    std::cout << "Center: " << center[0] << ", " << center[1] << ", " << center[2] << std::endl;

    // Create the axis actor
    vtkNew<vtkAxisActor> axis;
    axis->SetPoint1(-1.1, -1.0, -1.0);
    axis->SetPoint2(1.1, 1.0, 1.0);
    axis->SetTickLocationToBoth();
    // axis->SetAxisTypeToX();
    axis->SetTitle("A Sphere");
    axis->SetTitleScale(0.2);
    axis->TitleVisibilityOn();
    // axis->SetRange(0, 1);
    vtkProperty *majorProperty = axis->GetAxisMajorTicksProperty();
    majorProperty->SetLineWidth(1.5);
    double color[3] = {1, 0, 0};
    majorProperty->SetColor(color);
    majorProperty->PrintSelf(std::cout, vtkIndent(4));

    vtkProperty *minorProperty = axis->GetAxisMinorTicksProperty();
    minorProperty->SetLineWidth(0.5);
    color[0] = 0, color[1] = 1;
    minorProperty->SetColor(color);

    axis->SetMajorTickSize(0.15);
    axis->SetMinorTickSize(0.1);
    //    axis->TickVisibilityOn();
    //    axis->MinorTicksVisibleOn();
    // axis->DrawGridlinesOn();
    axis->GetTitleTextProperty()->SetColor(colors->GetColor3d("banana").GetData());
    axis->GetLabelTextProperty()->SetColor(colors->GetColor3d("orange").GetData());

    vtkNew<vtkStringArray> labels;
    labels->SetNumberOfTuples(1);
    labels->SetValue(0, "x Axis");

    axis->SetLabels(labels);
    axis->SetLabelScale(.1);
    //    axis->SetDeltaMajor(0, .001);
    // axis->SetCalculateTitleOffset(1);
    // axis->SetCalculateLabelOffset(1);

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(source->GetOutputPort());

    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);
    actor->GetProperty()->SetDiffuseColor(colors->GetColor4d("Tomato").GetData());

    // Create the RenderWindow, Renderer and both Actors
    vtkNew<vtkRenderer> renderer;
    vtkNew<vtkRenderWindow> renderWindow;
    renderWindow->AddRenderer(renderer);
    renderWindow->SetWindowName("AxisActor");

    vtkNew<vtkRenderWindowInteractor> interactor;
    interactor->SetRenderWindow(renderWindow);

    axis->SetCamera(renderer->GetActiveCamera());

    renderer->AddActor(actor);
    renderer->AddActor(axis);

    renderer->SetBackground(colors->GetColor4d("SlateGray").GetData());
    renderWindow->SetSize(640, 480);
    renderer->ResetCamera();
    renderer->ResetCameraClippingRange();

    // render the image
    renderWindow->Render();

    interactor->Initialize();
    interactor->Start();

    axis->PrintSelf(std::cout, vtkIndent(4));

    return EXIT_SUCCESS;
}
