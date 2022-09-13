#ifndef QtVTKRenderWindows_H
#define QtVTKRenderWindows_H
#include "vtkAxisActor2D.h"
#include "vtkDistanceWidget.h"
#include "vtkImagePlaneWidget.h"
#include "vtkResliceImageViewer.h"
#include "vtkResliceImageViewerMeasurements.h"
#include "vtkSmartPointer.h"
#include <QMainWindow>

// Forward Qt class declarations
class Ui_QtVTKRenderWindows;
class QtVTKRenderWindows : public QMainWindow {
    Q_OBJECT
  public:
    // Constructor/Destructor
    QtVTKRenderWindows(int argc, char *argv[]);
    ~QtVTKRenderWindows() override{};
    void initVtkAfterInitialization(char *argv[]);

  public Q_SLOTS:

    virtual void slotExit();
    virtual void resliceMode(int);
    virtual void thickMode(int);
    virtual void SetBlendModeToMaxIP();
    virtual void SetBlendModeToMinIP();
    virtual void SetBlendModeToMeanIP();
    virtual void SetBlendMode(int);
    virtual void ResetViews();
    virtual void Render();
    virtual void AddDistanceMeasurementToView1();
    virtual void AddDistanceMeasurementToView(int);
    virtual void AddDistanceMeasurementToMPRView(int);
    virtual void AddFixedDistanceMeasurementToView(int i);
    int addDistanceScale(const int sliceViewIdx);
    int addDistanceScaleV2(const int sliceViewIdx);
    int addDistanceScaleV3(const int sliceViewIdx);
    int addDistanceScaleV4(const int sliceViewIdx);
    void buildDistanceScaleRepresentation(const int sliceViewIdx);
    int addScale();

  public:
    vtkSmartPointer<vtkResliceImageViewer> m_riv[3];
    vtkSmartPointer<vtkImagePlaneWidget> planeWidget[3];
    vtkSmartPointer<vtkDistanceWidget> DistanceWidget[3 + 1];
    vtkSmartPointer<vtkResliceImageViewerMeasurements> ResliceMeasurements;
    vtkSmartPointer<vtkAxisActor2D> m_distanceScale;

  protected Q_SLOTS:

  private slots:
    // void on_AddDistance1Button_clicked();

  private:
    // Designer form
    Ui_QtVTKRenderWindows *ui;
    vtkSmartPointer<vtkImageData> m_data;
};

#endif // QtVTKRenderWindows_H
