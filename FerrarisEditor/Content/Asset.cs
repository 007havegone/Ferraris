using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;

namespace FerrarisEditor.Content
{
    enum AssetType
    {
        Unknown,
        Animation,
        Audio,
        Material,
        Mesh,
        Skeleton,
        Texture,
    }
    abstract class Asset : ViewModelBase
    {
        public static string AssetFileExtension => ".asset";
        public AssetType Type { get; private set; }

        public byte[] Icon { get; protected set; } 

        public string SourcePath { get; protected set; }

        public Guid Guid { get; protected set; } = new Guid();

        public DateTime ImportDate { get; protected set; }

        public byte[] Hash { get; protected set; } // this one is optional

        // The child implemnent this method to save the asset file.
        public abstract IEnumerable<string> Save(string file);

        protected void WriteAssetFileHeader(BinaryWriter writer)
        {
            var id = Guid.ToByteArray();
            var importDate = DateTime.Now.ToBinary();

            writer.BaseStream.Position = 0;
            writer.Write((int)Type);
            writer.Write(id.Length);
            writer.Write(id);
            writer.Write(importDate);
            if(Hash?.Length > 0)
            {
                writer.Write((int)Hash.Length);
                writer.Write(Hash);
            }
            else
            {
                writer.Write(0);
            }
            writer.Write(SourcePath ?? "");
            writer.Write(Icon.Length);
            writer.Write(Icon);
        }
        public Asset(AssetType type)
        {
            Debug.Assert(type != AssetType.Unknown);
            Type = type;
        }
    }
}