using FerrarisEditor.Utilities;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;

namespace FerrarisEditor.GameProject
{
    [DataContract]
    public class ProjectTemplate
    {
        [DataMember]
        public string ProjectType { get; set; }
        
        [DataMember]
        public string ProjectFile { get; set; }

        [DataMember]
        public List<string> Folders { get; set; }

    }
    class NewProject : ViewModelBase
    {
        // TODO: get the path from the installation location
        private readonly string _templatePath = @"..\..\FerrarisEditor\ProjectTemplates";
        private string _name = "NewProject";
        public string Name
        {
            get => _name;
            set
            {
                if (_name != value)
                {
                    _name = value;
                    OnPropertyChanged(nameof(Name));
                }
            }
        }
        // 设置默认路径
        private string _path = $@"{Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments)}\FerrarisProject\";
        public string Path
        {
            get => _path;
            set
            {
                if (_path != value)
                {
                    _path = value;
                    OnPropertyChanged(nameof(Path));
                }
            }
        }

        // bind the view
        private ObservableCollection<ProjectTemplate> _projectTemplates = new ObservableCollection<ProjectTemplate>();
        // expose it
        public ReadOnlyCollection<ProjectTemplate> ProjectTemplates
        { get; }

        public NewProject()
        {
            ProjectTemplates = new ReadOnlyCollection<ProjectTemplate>(_projectTemplates);
            try
            {
                var templatesFiles = Directory.GetFiles(_templatePath, "template.xml", SearchOption.AllDirectories);
                Debug.Assert(templatesFiles.Any());
                foreach (var file in templatesFiles)
                {
                    //var template = Serializer.FromFile<ProjectTemplate>(file);
                    //_projectTemplates.Add(template);
                    var template = new ProjectTemplate()
                    {
                        ProjectType = "Empty project",
                        ProjectFile = "project.ferraris",
                        Folders = new List<string>()
                        {
                            ".ferraris",
                            "Content",
                            "GameCode"
                        }
                    };
                    Serializer.ToFile<ProjectTemplate>(template, file);
                }
            }
            catch(Exception ex)
            {
                Debug.WriteLine(ex.Message);
                // TODO: log err
            }
        }
    }
    
}
