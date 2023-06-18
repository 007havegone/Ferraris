using FerrarisEditor.Content;
using System;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Media3D;

namespace FerrarisEditor.Editors
{
    /**
     * The purpose of this calss is to enable viewing 3D geometry in WPF.
     * This is temporary solution, while we finish a graphics renderer in game engine,
     * this method will become obselete.
     */
    class MeshRendererVertexData : ViewModelBase
    {
        // Following Brush used as a Materials.
        private Brush _specular = new SolidColorBrush((Color)ColorConverter.ConvertFromString("#ff111111"));
        public Brush Specular
        {
            get => _specular;

            set
            {
                if(_specular != value)
                {
                    _specular = value;
                    OnPropertyChanged(nameof(Specular));
                }
            }
        }

        private Brush _diffuse = Brushes.White;
        public Brush Diffuse
        {
            get => _diffuse;

            set
            {
                if (_diffuse != value)
                {
                    _diffuse = value;
                    OnPropertyChanged(nameof(Diffuse));
                }
            }
        }
        public Point3DCollection Positions { get; } = new Point3DCollection();
        public Vector3DCollection Normals { get; } = new Vector3DCollection();
        public PointCollection UVs { get; } = new PointCollection();

        public Int32Collection Indices { get; } = new Int32Collection();

    }
    /**
     * The purpose of this calss is to enable viewing 3D geometry in WPF.
     * This is temporary solution, while we finish a graphics renderer in game engine,
     * this method will become obselete.
     */
    class MeshRenderer : ViewModelBase
    {
        public ObservableCollection<MeshRendererVertexData> Meshes { get; } = new ObservableCollection<MeshRendererVertexData>();

        // Property for the camera
        private Vector3D _cameraDirection = new Vector3D(0, 0, -10);
        public Vector3D CameraDirection
        {
            get => _cameraDirection;
            set
            {
                if (_cameraDirection != value)
                {
                    _cameraDirection = value;
                    OnPropertyChanged(nameof(CameraDirection));
                }
            }
        }

        private Point3D _cameraPosition = new Point3D(0, 0, 10);
        public Point3D CameraPosition
        {
            get => _cameraPosition;
            set
            {
                if (_cameraPosition != value)
                {
                    _cameraPosition = value;
                    CameraDirection = new Vector3D(-value.X, -value.Y, -value.Z);
                    OnPropertyChanged(nameof(OffsetCameraPosition));
                    OnPropertyChanged(nameof(CameraPosition));
                }
            }
        }
        private Point3D _cameraTarget = new Point3D(0, 0, 0);
        public Point3D CameraTarget
        {
            get => _cameraTarget;
            set
            {
                if (_cameraTarget != value)
                {
                    _cameraTarget = value;
                    OnPropertyChanged(nameof(OffsetCameraPosition));
                    OnPropertyChanged(nameof(CameraTarget));
                }
            }
        }

        public Point3D OffsetCameraPosition =>
            new Point3D(CameraPosition.X + CameraTarget.X, CameraPosition.Y + CameraTarget.Y, CameraPosition.Z + CameraTarget.Z);

        // Some Lights from the ground and scene
        private Color _keyLight = (Color)ColorConverter.ConvertFromString("#ffaeaeae");
        public Color KeyLight
        {
            get => _keyLight;
            set
            {
                if (_keyLight != value)
                {
                    _keyLight = value;
                    OnPropertyChanged(nameof(KeyLight));
                }
            }
        }

        private Color _skyLight = (Color)ColorConverter.ConvertFromString("#ff111b30");
        public Color SkyLight
        {
            get => _skyLight;
            set
            {
                if (_skyLight != value)
                {
                    _skyLight = value;
                    OnPropertyChanged(nameof(SkyLight));
                }
            }
        }

        private Color _groundLight = (Color)ColorConverter.ConvertFromString("#ff3f2f1e");
        public Color GroundLight
        {
            get => _groundLight;
            set
            {
                if (_groundLight != value)
                {
                    _groundLight = value;
                    OnPropertyChanged(nameof(GroundLight));
                }
            }
        }

        private Color _ambientLight = (Color)ColorConverter.ConvertFromString("#ff3b3b3b");
        public Color AmbientLight
        {
            get => _ambientLight;
            set
            {
                if (_ambientLight != value)
                {
                    _ambientLight = value;
                    OnPropertyChanged(nameof(AmbientLight));
                }
            }
        }


        [Obsolete("if the project update .NET 6, this function will remove")]
        private float Clamp(float Value, float minValue, float maxValue)
        {
            if (Value < minValue)
                return minValue;
            else if (Value > maxValue)
                return maxValue;
            else
                return Value;
        }

        public MeshRenderer(MeshLOD lod, MeshRenderer old)
        {
            Debug.Assert(lod?.Meshes.Any() == true);

            // Get the offset of tangent in the vertex_static, calculated by vertex size minus the position and nromal vectors
            var offset = lod.Meshes[0].VertexSize - 3 * sizeof(float) - sizeof(int) - 2 * sizeof(short);
            // Calculate the bounding Box of the objects,
            // which help us to set the Camera position and target property.
            double minX, minY, minZ; minX = minY = minZ = double.MaxValue;
            double maxX, maxY, maxZ; maxX = maxY = maxZ = double.MinValue;
            Vector3D avgNormal = new Vector3D();
            var intervals = 2.0f / ((1 << 16) - 1);
            foreach (var mesh in lod.Meshes)
            {
                var vertexData = new MeshRendererVertexData();
                // Unpack all vertices
                using (var reader = new BinaryReader(new MemoryStream(mesh.Vertices)))
                    for (int i = 0; i < mesh.VertexCount; ++i)
                    {
                        // Read the position
                        var posX = reader.ReadSingle();
                        var posY = reader.ReadSingle();
                        var posZ = reader.ReadSingle();
                        var signs = (reader.ReadUInt32() >> 24) & 0x000000ff; // get the last byte
                        vertexData.Positions.Add(new Point3D(posX, posY, posZ));

                        // Adjust the bounding box
                        minX = Math.Min(minX, posX); maxX = Math.Max(maxX, posX);
                        minY = Math.Min(minY, posY); maxY = Math.Max(maxY, posY);
                        minZ = Math.Min(minZ, posZ); maxZ = Math.Max(maxZ, posZ);

                        // Read Normals, the details compute refs to the unpack_to_float in Math.h
                        var nrmX = reader.ReadUInt16() * intervals - 1.0f;
                        var nrmY = reader.ReadUInt16() * intervals - 1.0f;
                        var nrmZ = Math.Sqrt(Clamp(1f - (nrmX * nrmX + nrmY * nrmY), 0f, 1f)) * ((signs & 0x2) - 1f);
                        var normal = new Vector3D(nrmX, nrmY, nrmZ);
                        normal.Normalize();
                        vertexData.Normals.Add(normal);
                        avgNormal += normal;
                        // Read UVs
                        reader.BaseStream.Position += (offset - sizeof(float) * 2);
                        var u = reader.ReadSingle();
                        var v = reader.ReadSingle();
                        vertexData.UVs.Add(new Point(u, v));
                    }
                using (var reader = new BinaryReader(new MemoryStream(mesh.Indices)))
                    if (mesh.IndexSize == sizeof(short))
                        for (int i = 0; i < mesh.IndexCount; ++i) vertexData.Indices.Add(reader.ReadUInt16());
                    else
                        for (int i = 0; i < mesh.IndexCount; ++i) vertexData.Indices.Add(reader.ReadInt32());

                // done unpacking the data
                // Freeze the Object, this object does not modify, the framework will do optimization
                vertexData.Positions.Freeze();
                vertexData.Normals.Freeze();
                vertexData.UVs.Freeze();
                vertexData.Indices.Freeze();
                Meshes.Add(vertexData);

            }
            // set the camera target and position
            if (old != null)
            {
                CameraTarget = old.CameraTarget;
                CameraPosition = old.CameraPosition;
            }
            else
            {
                //  compute bouding box dimensions
                var width = maxX - minX;
                var height = maxY - minY;
                var depth = maxZ - minZ;
                var radius = new Vector3D(height, width, depth).Length * 1.2;
                // Heuristic value for the normal direction
                if (avgNormal.Length > 0.8)
                {
                    avgNormal.Normalize();
                    avgNormal *= radius;
                    CameraPosition = new Point3D(avgNormal.X, avgNormal.Y, avgNormal.Z);
                }
                else
                {
                    CameraPosition = new Point3D(width, height * 0.5, radius);

                }
                CameraTarget = new Point3D(minX + width * 0.5, minY + height * 0.5, minZ + depth * 0.5);
            }

        }
    }


    class GeometryEditor : ViewModelBase, IAssetEditor
    {
        public Content.Asset Asset => Geometry;

        public Content.Geometry _geometry;

        public Content.Geometry Geometry
        {
            get => _geometry;

            set
            {
                if(_geometry != value)
                {
                    _geometry = value;
                    OnPropertyChanged(nameof(Geometry));
                }
            }
        }
        
        private MeshRenderer _meshRenderer;
        public MeshRenderer MeshRenderer
        {
            get => _meshRenderer;

            set
            {
                if (_meshRenderer != value)
                {
                    _meshRenderer = value;
                    OnPropertyChanged(nameof(MeshRenderer));
                }
            }
        }
        public void SetAsset(Content.Asset asset)
        {
            Debug.Assert(asset is Content.Geometry);

            if(asset is Content.Geometry geometry)
            {
                Geometry = geometry;
                // now we only render the Lod 0
                MeshRenderer = new MeshRenderer(Geometry.GetLODGroup().LODs[0], MeshRenderer);
            }
        }

    }
}
