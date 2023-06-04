using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Media.Media3D;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace FerrarisEditor.Editors
{
    /// <summary>
    /// Interaction logic for GeometryView.xaml
    /// </summary>
    public partial class GeometryView : UserControl
    {

        public void SetGeometry(int index = -1)
        {
            if (!(DataContext is MeshRenderer vm)) return;

            // We add the ModelVisaul3D at this viewport at the end of this method.
            // once update remove it and add new one.
            if(vm.Meshes.Any() && viewport.Children.Count == 2)
            {
                viewport.Children.RemoveAt(1);
            }

            var meshIndex = 0;
            var modelGroup = new Model3DGroup();
            foreach(var mesh in vm.Meshes)
            {
                if(index != -1 && meshIndex != index)
                {
                    ++meshIndex;
                    continue;
                }
                // get the data from the DataContext to create the Geometry
                var mesh3D = new MeshGeometry3D()
                {
                    Positions = mesh.Positions,
                    Normals = mesh.Normals,
                    TriangleIndices = mesh.Indices,
                    TextureCoordinates = mesh.UVs
                };

                var diffuse = new DiffuseMaterial(mesh.Diffuse);
                var specular = new SpecularMaterial(mesh.Specular,50);
                var matGroup = new MaterialGroup();
                matGroup.Children.Add(diffuse);
                matGroup.Children.Add(specular);

                // use the Mesh and Light to create the model
                var model = new GeometryModel3D(mesh3D, matGroup);
                modelGroup.Children.Add(model);
                // binding the materials with the mesh, once if the light is changed, the mesh of model will
                // change in the viewer.
                var binding = new Binding(nameof(mesh.Diffuse)) { Source = mesh };
                BindingOperations.SetBinding(diffuse, DiffuseMaterial.BrushProperty, binding);

                if (meshIndex == index) break;
            }

            var visual = new ModelVisual3D() { Content = modelGroup };
            viewport.Children.Add(visual);
        }
        public GeometryView()
        {
            InitializeComponent();
            DataContextChanged += (s, e) => SetGeometry();
        }
    }
}
