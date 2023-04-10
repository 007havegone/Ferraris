using FerrarisEditor.ContentToolsAPIStructs;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace FerrarisEditor.ContentToolsAPIStructs
{
  
    [StructLayout(LayoutKind.Sequential)]
    class GeometryImportSettings
    {
        public float SmoothingAngle = 178f;
        public byte CalculateNormals = 0;
        public byte CalculateTangents = 1;
        public byte ReverseHandedness = 0;
        public byte ImportEmbededTexture = 1;
        public byte ImportAnimation = 1;
    }
    [StructLayout(LayoutKind.Sequential)]
    class SceneData
    {
        public IntPtr Buffer;
        public int BufferSize;
        public GeometryImportSettings ImportSettings = new GeometryImportSettings();
    }

    [StructLayout(LayoutKind.Sequential)]
     class PrimitiveInitInfo
    {
        public Content.PrimitiveMeshType Type;
        public int SegmentX = 1;
        public int SegmentY = 1;
        public int SegmentZ = 1;
        public Vector3 Size = new Vector3(1f);
        public int LOD = 0;

    }
    
}
namespace FerrarisEditor.DllWarpper
{
    static class ContentToolsAPI
    {
        private const string _toolsDll = "ContentTools.dll";

        [DllImport(_toolsDll)]
        private static extern void CreatePrimitiveMesh([In, Out] SceneData data, PrimitiveInitInfo info);
    }
}
