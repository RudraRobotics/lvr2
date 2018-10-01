#include <QFileDialog>
#include "LVROptimizationPlanarOptimizationDialog.hpp"

#include <lvr2/algorithm/CleanupAlgorithms.hpp>
#include <lvr2/algorithm/ClusterAlgorithms.hpp>
#include <lvr2/algorithm/FinalizeAlgorithms.hpp>
#include <lvr2/algorithm/NormalAlgorithms.hpp>
#include <lvr2/algorithm/Tesselator.hpp>

#include <lvr2/geometry/BaseVector.hpp>
#include <lvr2/geometry/HalfEdgeMesh.hpp>

#include <lvr2/io/Model.hpp>

#include <lvr2/util/ClusterBiMap.hpp>

namespace lvr2
{

LVRPlanarOptimizationDialog::LVRPlanarOptimizationDialog(LVRMeshItem* mesh, LVRModelItem* parent, QTreeWidget* treeWidget, vtkRenderWindow* window) :
   m_mesh(mesh), m_parent(parent), m_treeWidget(treeWidget), m_renderWindow(window)
{
    // Setup DialogUI and events
    QDialog* dialog = new QDialog(m_treeWidget);
    m_dialog = new PlanarOptimizationDialog;
    m_dialog->setupUi(dialog);

    connectSignalsAndSlots();

    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}

LVRPlanarOptimizationDialog::~LVRPlanarOptimizationDialog()
{
    // TODO Auto-generated destructor stub
}

void LVRPlanarOptimizationDialog::connectSignalsAndSlots()
{
    QObject::connect(m_dialog->buttonBox, SIGNAL(accepted()), this, SLOT(optimizeMesh()));
    QObject::connect(m_dialog->checkBox_sr, SIGNAL(stateChanged(int)), this, SLOT(toggleSmallRegionRemoval(int)));
    QObject::connect(m_dialog->checkBox_rt, SIGNAL(stateChanged(int)), this, SLOT(toggleRetesselation(int)));
}

void LVRPlanarOptimizationDialog::toggleSmallRegionRemoval(int state)
{
    QSpinBox* smallRegionRemoval_box = m_dialog->spinBox_sr;
    if(state == Qt::Checked)
    {
        smallRegionRemoval_box->setEnabled(true);
    }
    else
    {
        smallRegionRemoval_box->setEnabled(false);
    }
}

void LVRPlanarOptimizationDialog::toggleRetesselation(int state)
{
    QCheckBox* generateTextures_box = m_dialog->checkBox_gt;
    QDoubleSpinBox* lineSegmentThreshold_box = m_dialog->doubleSpinBox_ls;
    if(state == Qt::Checked)
    {
        generateTextures_box->setEnabled(true);
        lineSegmentThreshold_box->setEnabled(true);
    }
    else
    {
        generateTextures_box->setEnabled(false);
        lineSegmentThreshold_box->setEnabled(false);
    }
}

void LVRPlanarOptimizationDialog::optimizeMesh()
{
    QSpinBox* planeIterations_box = m_dialog->spinBox_pi;
    int planeIterations = planeIterations_box->value();
    QDoubleSpinBox* normalThreshold_box = m_dialog->doubleSpinBox_nt;
    float normalThreshold = (float)normalThreshold_box->value();
    QSpinBox* minimalPlaneSize_box = m_dialog->spinBox_mp;
    int minimalPlaneSize = minimalPlaneSize_box->value();
    QCheckBox* removeSmallRegions_box = m_dialog->checkBox_sr;
    bool removeSmallRegions = removeSmallRegions_box->isChecked();
    QSpinBox* removeSmallRegionThreshold_box = m_dialog->spinBox_sr;
    int removeSmallRegionThreshold = (removeSmallRegions) ? removeSmallRegionThreshold_box->value() : 0;
    QCheckBox* fillHoles_box = m_dialog->checkBox_fh;
    bool fillHoles = fillHoles_box->isChecked();
    QCheckBox* retesselate_box = m_dialog->checkBox_rt;
    bool retesselate = retesselate_box->isChecked();
    QCheckBox* generateTextures_box = m_dialog->checkBox_gt;
    bool generateTextures = generateTextures_box->isChecked();
    QDoubleSpinBox* lineSegmentThreshold_box = m_dialog->doubleSpinBox_ls;
    float lineSegmentThreshold = (float)lineSegmentThreshold_box->value();

    using Vec = BaseVector<float>;

    HalfEdgeMesh<Vec> mesh(m_mesh->getMeshBuffer());

    if (fillHoles)
    {
        // variable for max_size?
        // TODO happended in old code right after deletesmallPlanarCluster.
        //int res = naiveFillSmallHoles(mesh, 1);
    }

    auto faceNormals = calcFaceNormals(mesh);
    ClusterBiMap<FaceHandle> clusterBiMap = iterativePlanarClusterGrowing(
        mesh, 
        faceNormals,
        normalThreshold, 
        planeIterations,
        minimalPlaneSize
    );

    if (removeSmallRegions && removeSmallRegionThreshold > 0)
    {
       deleteSmallPlanarCluster(mesh, clusterBiMap, removeSmallRegionThreshold);
    }

     
    // TODO idk what the lvr2 equivalent to this is?
    //mesh.restorePlanes(minimalPlaneSize);

    // Save triangle mesh
    if(retesselate)
    {
        Tesselator<Vec>::apply(mesh, clusterBiMap, faceNormals, lineSegmentThreshold);
    }
    
    MeshBuffer2Ptr res; 
    if (generateTextures)
    {
        // TODO Use TextureFinalizer... 
        SimpleFinalizer<Vec> fin;
        res = fin.apply(mesh);
    }
    else
    {
        SimpleFinalizer<Vec> fin;
        res = fin.apply(mesh);
    }

    ModelPtr model(new Model(res));
    ModelBridgePtr bridge(new LVRModelBridge(model));
    vtkSmartPointer<vtkRenderer> renderer = m_renderWindow->GetRenderers()->GetFirstRenderer();
    bridge->addActors(renderer);

    QString base = m_parent->getName() + " (optimized)";
    m_optimizedModel = new LVRModelItem(bridge, base);

    m_treeWidget->addTopLevelItem(m_optimizedModel);
    m_optimizedModel->setExpanded(true);
}

} // namespace lvr2
