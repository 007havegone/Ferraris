﻿using System;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Media.Media3D;

namespace FerrarisEditor.Editors
{
    /// <summary>
    /// Interaction logic for GeometryView.xaml
    /// </summary>
    public partial class GeometryView : UserControl
    {
        private static readonly GeometryView _geometryView = new GeometryView() { Background = (Brush)Application.Current.FindResource("Editor.Window.GrayBrush4") };
        private Point _clickedPosition;
        private bool _captureLeft;
        private bool _captureRight;
        public void SetGeometry(int index = -1)
        {
            if (!(DataContext is MeshRenderer vm)) return;

            // We add the ModelVisaul3D at this viewport at the end of this method.
            // once update remove it and add new one.
            if (vm.Meshes.Any() && viewport.Children.Count == 2)
            {
                viewport.Children.RemoveAt(1);
            }

            var meshIndex = 0;
            var modelGroup = new Model3DGroup();
            foreach (var mesh in vm.Meshes)
            {
                if (index != -1 && meshIndex != index)
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
                var specular = new SpecularMaterial(mesh.Specular, 50);
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

        private void OnGrid_Mouse_LBD(object sender, MouseButtonEventArgs e)
        {
            _clickedPosition = e.GetPosition(this);
            _captureLeft = true;
            Mouse.Capture(sender as UIElement);
        }

        private void OnGrid_MouseMove(object sender, MouseEventArgs e)
        {
            if (!_captureLeft && !_captureRight) return;

            var pos = e.GetPosition(this);
            var d = pos - _clickedPosition;

            if (_captureLeft && !_captureRight)
            {
                MoveCamera(d.X, d.Y, 0);
            }
            else if (!_captureLeft && _captureRight)
            {
                var vm = DataContext as MeshRenderer;
                var cp = vm.CameraDirection;
                var yOffset = d.Y * 0.001 * Math.Sqrt(cp.X * cp.X + cp.Z * cp.Z);
                vm.CameraTarget = new Point3D(vm.CameraTarget.X, vm.CameraTarget.Y + yOffset, vm.CameraTarget.Z);
            }

            _clickedPosition = pos;
        }

        private void OnGrid_Mouse_LBU(object sender, MouseButtonEventArgs e)
        {
            _captureLeft = false;
            if (!_captureRight) Mouse.Capture(null);
        }

        private void OnGrid_MouseWheel(object sender, MouseWheelEventArgs e)
        {
            MoveCamera(0, 0, Math.Sign(e.Delta));
        }

        private void OnGrid_Mouse_RBD(object sender, MouseButtonEventArgs e)
        {
            _clickedPosition = e.GetPosition(this);
            _captureRight = true;
            Mouse.Capture(sender as UIElement);

        }

        private void OnGrid_Mouse_RBU(object sender, MouseButtonEventArgs e)
        {
            _captureRight = false;
            if (!_captureLeft) Mouse.Capture(null);
        }
        private void MoveCamera(double dx, double dy, int dz)
        {
            var vm = DataContext as MeshRenderer;
            var v = new Vector3D(vm.CameraPosition.X, vm.CameraPosition.Y, vm.CameraPosition.Z);

            var r = v.Length;
            var thera = Math.Acos(v.Y / r);
            var phi = Math.Atan2(-v.Z, v.X);

            thera -= dy * 0.01;
            phi -= dx * 0.01;
            r *= 1.0 - 0.1 * dz; // dz is either +1 or -1 or 0

            thera = thera < 0.0001 ? 0.0001 : (thera > Math.PI - 0.0001 ? Math.PI - 0.0001 : thera);


            v.X = r * Math.Sin(thera) * Math.Cos(phi);
            v.Z = -r * Math.Sin(thera) * Math.Sin(phi);
            v.Y = r * Math.Cos(thera);

            vm.CameraPosition = new Point3D(v.X, v.Y, v.Z);

        }
        internal static BitmapSource RenderToBitmap(MeshRenderer mesh, int width, int height)
        {
            var bmp = new RenderTargetBitmap(width, height, 96, 96, PixelFormats.Default);

            // create the GeometryView for creating the asset icon
            _geometryView.DataContext = mesh;
            _geometryView.Width = width;
            _geometryView.Height = height;
            _geometryView.Measure(new Size(width, height));
            _geometryView.Arrange(new Rect(0, 0, width, height));
            _geometryView.UpdateLayout();

            bmp.Render(_geometryView);
            return bmp;
        }

        public GeometryView()
        {
            InitializeComponent();
            DataContextChanged += (s, e) => SetGeometry();
        }
    }
}
