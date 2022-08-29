import vtk

def setDicomPath():
    #TODO : setup a dialog box to set fileName
    fileName = 'pathToDICOMFolder'
    fileName = u"/media/eton/statics/08.dicom-datas/BRAINIX/BRAINIX/MRs/T1-3D-FFE-C-801/"
    print(fileName)
    return fileName


if __name__=="__main__":
    # thresholds for volume rendering
    thresh1 = 150
    thresh2 = 320
    thresh3 = 440

    nbDim = 3

    #Reading dicom
    myFile = setDicomPath()
    readDICOM = vtk.vtkDICOMImageReader()
    readDICOM.SetDirectoryName(myFile)
    readDICOM.Update()
    myDicom = readDICOM.GetOutput()

    wholeExtent = myDicom.GetDimensions()
    scalarRange = myDicom.GetScalarRange()

    # Define main Renderer window and interactor
    renWin = vtk.vtkRenderWindow()
    renWin.SetSize(1000,1000)
    renWin.BordersOn()

    iren = vtk.vtkRenderWindowInteractor()
    # irenStyle = vtk.vtkInteractorStyleImage()
    irenStyle = vtk.vtkInteractorStyleTrackballCamera()
    iren.SetInteractorStyle(irenStyle)
    picker = vtk.vtkCellPicker()
    picker.SetTolerance(0.005)
    iren.SetRenderWindow(renWin)
    iren.SetPicker(picker)
    iren.GetPickingManager().SetEnabled(1)
    iren.GetPickingManager().AddPicker(picker)
    iren.Initialize()

    # Define viewport ranges in renderwindow
    xmins = [0.00, 0.51, 0.00, .51]
    xmaxs = [0.49, 1.00, 0.49, 1]
    ymins = [0.00, 0.00, 0.51, .51]
    ymaxs = [0.49, 0.49, 1.00, 1]

    # TODO : confirm if this camera orientation is good for 3D echo
    viewUp = [[0, 0, -1],[0, 0, -1],[0, 1, 0]]

    # Define 3 MPR views using image plane widgets and reslice cursor
    ipws = []
    rens = []
    rcws = []
    rcwReps = []

    # First renderer, used to display volume3D or slice planes in 3D
    firstRen = vtk.vtkRenderer()
    rens.append(firstRen)
    renWin.AddRenderer(firstRen)
    firstRen.SetViewport(xmins[3], ymins[3], xmaxs[3], ymaxs[3])

    # Reslice cursor generating the 3 slice planes
    resliceCursor = vtk.vtkResliceCursor()
    resliceCursor.SetCenter(myDicom.GetCenter())
    resliceCursor.SetThickMode(3)
    resliceCursor.SetThickness(0, 0, 0)# if it eq 0 then only one line display.
    resliceCursor.SetHole(1)
    resliceCursor.SetHoleWidthInPixels(198)
    print("Hole in relice cursor=", resliceCursor.GetHole(), ",width=", resliceCursor.GetHoleWidth(), ",width(pixel)=", resliceCursor.GetHoleWidthInPixels())
    resliceCursor.SetImage(myDicom)

    for i in range(nbDim):
        # One renderer for each slice orientation
        ren = vtk.vtkRenderer()
        rens.append(ren)
        renWin.AddRenderer(ren)
        ren.SetViewport(xmins[i], ymins[i], xmaxs[i], ymaxs[i])

        # One vtkResliceCursorWidget for each slice orientation, based on common Reslice cursor
        rcw = vtk.vtkResliceCursorWidget()
        rcws.append(rcw)
        rcw.SetInteractor(iren)

        rcwRep = vtk.vtkResliceCursorThickLineRepresentation()
        rcwReps.append(rcwRep)
        rcwRep.SetRestrictPlaneToVolume(1)
        rcw.SetRepresentation(rcwRep)
        rcwRep.GetResliceCursorActor().GetCursorAlgorithm().SetResliceCursor(resliceCursor)
        directionMsc= i % 2
        rcwRep.GetResliceCursorActor().GetCursorAlgorithm().SetReslicePlaneNormal(directionMsc)
        rcwRep.ManipulationMode = 0

        rcw.SetDefaultRenderer(ren)
        rcw.SetEnabled(1)
        bgcolor=[0.1,0.1,0.1]
        bgcolor[i]=1
        ren.SetBackground(bgcolor)

        # Setting right camera orientation
        ren.GetActiveCamera().SetFocalPoint(0, 0, 0)
        camPos = [0, 0, 0]
        camPos[i] = directionMsc
        ren.GetActiveCamera().SetPosition(camPos)
        #ren.GetActiveCamera().ParallelProjectionOn()
        ren.GetActiveCamera().SetViewUp(viewUp[i])
        ren.ResetCamera()

        # Initialize the window level to a sensible value
        rcwRep.SetWindowLevel(scalarRange[1] - scalarRange[0], (scalarRange[0] + scalarRange[1]) / 2.0)

        # Make all slice plane share the same color map.
        rcwRep.SetLookupTable(rcwReps[0].GetLookupTable())

        rcw.On()

    # 3D Raycast Viewer
    colorTransferFunction = vtk.vtkColorTransferFunction()
    colorTransferFunction.AddRGBPoint(scalarRange[0], 0.0, 0.0, 0.0)
    colorTransferFunction.AddRGBPoint(thresh1, 140/255, 64/255, 38/255)
    colorTransferFunction.AddRGBPoint(thresh2, 225/255, 154/255, 74/255)
    colorTransferFunction.AddRGBPoint(thresh3, 255/255, 239/255, 243/255)
    colorTransferFunction.AddRGBPoint(scalarRange[1], 211/255, 168/255, 255/255)

    funcOpacityScalar = vtk.vtkPiecewiseFunction()
    funcOpacityScalar.AddPoint(scalarRange[0],   0)
    funcOpacityScalar.AddPoint(thresh1,   0)
    funcOpacityScalar.AddPoint(thresh2,   0.45)
    funcOpacityScalar.AddPoint(thresh3,   0.63)
    funcOpacityScalar.AddPoint(scalarRange[1],   0.63)

    volumeMapper = vtk.vtkGPUVolumeRayCastMapper()
    volumeMapper.SetInputData(myDicom)
    volumeMapper.SetBlendModeToComposite()
    volumeMapper.AutoAdjustSampleDistancesOn()

    volumeProperty = vtk.vtkVolumeProperty()
    volumeProperty.ShadeOn()
    volumeProperty.SetScalarOpacity(funcOpacityScalar)
    volumeProperty.SetInterpolationTypeToLinear()
    volumeProperty.SetColor(colorTransferFunction)
    volumeProperty.SetAmbient(0.20)
    volumeProperty.SetDiffuse(1.00)
    volumeProperty.SetSpecular(0.00)
    volumeProperty.SetSpecularPower(0.00)

    actorVolume = vtk.vtkVolume()
    actorVolume.SetMapper(volumeMapper)
    actorVolume.SetProperty(volumeProperty)# Define renderer for Volume

    rens[0].SetBackground(255/255, 255/255, 255/255)
    rens[0].AddVolume(actorVolume)
    rens[0].ResetCameraClippingRange()

    renWin.Render()

    iren.Start()

