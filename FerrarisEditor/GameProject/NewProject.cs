using FerrarisEditor.Utilities;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.Serialization;

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

        public byte[] Icon { get; set; }

        public byte[] Screenshot { get; set; }

        public string IconFilePath { get; set; }

        public string ScreenshotFilePath { get; set; }

        public string ProjectFilePath { get; set; }
        public string TemplatePath { get; set; }
    }
    class NewProject : ViewModelBase
    {
        // TODO: get the path from the installation location
        private readonly string _templatePath = @"..\..\FerrarisEditor\ProjectTemplates";
        private string _projectName = "NewProject";
        public string ProjectName // 面板中的项目名
        {
            get => _projectName;
            set
            {
                if (_projectName != value)
                {
                    _projectName = value;
                    ValidateProjectPath();// check the project path
                    OnPropertyChanged(nameof(ProjectName));
                }
            }
        }
        // 设置默认路径
        private string _projectPath = $@"{Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments)}\FerrarisProject\";
        public string ProjectPath // 面板中的项目路径
        {
            get => _projectPath;
            set
            {
                if (_projectPath != value)
                {
                    _projectPath = value;
                    OnPropertyChanged(nameof(ProjectPath));
                }
            }
        }
        private bool _isValid = false;

        public bool IsValid
        {
            get => _isValid;

            set
            {
                if(_isValid != value)
                {
                    _isValid = value;
                    OnPropertyChanged(nameof(IsValid));
                }
            }
        }
        private string _errorMsg;
        public string ErrorMsg
        {
            get => _errorMsg;

            set
            {
                if (_errorMsg != value)
                {
                    _errorMsg = value;
                    OnPropertyChanged(nameof(ErrorMsg));
                }
            }
        }


        // bind the view
        private ObservableCollection<ProjectTemplate> _projectTemplates = new ObservableCollection<ProjectTemplate>();
        // expose it
        public ReadOnlyCollection<ProjectTemplate> ProjectTemplates
        { get; }

        private bool ValidateProjectPath()
        {
            var path = ProjectPath;

            if (path.LastOrDefault() !=  Path.DirectorySeparatorChar) path += @"\";// realize the .net 7 EndsInDirectorySeparator
            path += $@"{ProjectName}\";// 组合得到真正的项目路径

            IsValid = false;
            // project Name error
            if(string.IsNullOrWhiteSpace(ProjectName.Trim()))
            {
                ErrorMsg = "Type in a project name.";
            }
            else if(ProjectName.IndexOfAny(Path.GetInvalidFileNameChars()) != -1)
            {
                ErrorMsg = "Invalid character(s) used in project name.";
            }
            // project Path error
            else if(string.IsNullOrWhiteSpace(ProjectPath.Trim()))
            {
                ErrorMsg = "Select a valid project folder.";
            }
            else if(ProjectPath.IndexOfAny(Path.GetInvalidPathChars()) != -1)
            {
                ErrorMsg = "Invalid character(s) used in project path.";
            }
            else if(Directory.Exists(path) && Directory.EnumerateFileSystemEntries(path).Any())
            {
                ErrorMsg = "Selected project folder already exists and is not empty";
            }
            else
            {
                ErrorMsg = string.Empty;
                IsValid = true;
            }
            return IsValid;
        }

        public string CreateProject(ProjectTemplate template)
        {
            ValidateProjectPath();
            if(!IsValid)
            {
                return string.Empty;
            }

             if (ProjectPath.Last() != '\\') ProjectPath += @"\";
            var path = $@"{ProjectPath}{ProjectName}\";//full path of proejct

            try
            {
                if (!Directory.Exists(path)) Directory.CreateDirectory(path);
                foreach(var folder in template.Folders)
                {
                    Directory.CreateDirectory(Path.GetFullPath(Path.Combine(Path.GetDirectoryName(path), folder)));
                }
                var dirInfo = new DirectoryInfo(path + @".Ferraris\");
                dirInfo.Attributes |= FileAttributes.Hidden;// hide this directory
                File.Copy(template.IconFilePath, Path.GetFullPath(Path.Combine(dirInfo.FullName, "icon.png")));
                File.Copy(template.ScreenshotFilePath, Path.GetFullPath(Path.Combine(dirInfo.FullName, "Screenshot.png")));

                // now we serialize the data of project, for later reuse.
                //var project = new Project(ProjectName, path);
                //Serializer.ToFile(project, path + $"{ProjectName}"+ Project.Extension);
                var projectXml = File.ReadAllText(template.ProjectFilePath);
                projectXml = string.Format(projectXml, ProjectName, path);// fill the placeholder in the file
                var projectPath = Path.GetFullPath(Path.Combine(path, $"{ProjectName}{Project.Extension}"));
                File.WriteAllText(projectPath, projectXml);// copy the project.ferraris file from default template

                CreateMSVCSolution(template, path);
                return path;
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
                Logger.Log(MessageType.Error, $"Failed to create {ProjectName}");
                throw;
            }

        }

        private void CreateMSVCSolution(ProjectTemplate template, string projectPath)
        {
            Debug.Assert(File.Exists(Path.Combine(template.TemplatePath, "MSVCSolution")));
            Debug.Assert(File.Exists(Path.Combine(template.TemplatePath, "MSVCProject")));

            var engineAPIPath = Path.Combine(MainWindow. FerrarisPath, @"Engine\EngineAPI");
            Debug.Assert(Directory.Exists(engineAPIPath));

            var _0 = ProjectName;
            var _1 = "{" + Guid.NewGuid().ToString().ToUpper() + "}";
            var _2 = engineAPIPath;
            var _3 = MainWindow.FerrarisPath;

            var solution = File.ReadAllText(Path.Combine(template.TemplatePath, "MSVCSolution"));
            solution = string.Format(solution, _0, _1, "{" + Guid.NewGuid().ToString().ToUpper() + "}");
            File.WriteAllText(Path.GetFullPath(Path.Combine(projectPath, $"{_0}.sln")), solution);

            var project = File.ReadAllText(Path.Combine(template.TemplatePath, "MSVCProject"));
            project = string.Format(project, _0, _1, _2, _3);
            File.WriteAllText(Path.GetFullPath(Path.Combine(projectPath, $@"GameCode\{_0}.vcxproj")), project);
        }

        public NewProject()
        {
            ProjectTemplates = new ReadOnlyCollection<ProjectTemplate>(_projectTemplates);
            try
            {
                var templatesFiles = Directory.GetFiles(_templatePath, "template.xml", SearchOption.AllDirectories);
                Debug.Assert(templatesFiles.Any());
                foreach (var file in templatesFiles)
                {
                    // read the project configuration from the template.xml file
                    var template = Serializer.FromFile<ProjectTemplate>(file);
                    template.TemplatePath = Path.GetDirectoryName(file);
                    template.IconFilePath = Path.GetFullPath(Path.Combine(template.TemplatePath, "icon.png"));
                    template.Icon = File.ReadAllBytes(template.IconFilePath);
                    template.ScreenshotFilePath = Path.GetFullPath(Path.Combine(template.TemplatePath, "Screenshot.png"));
                    template.Screenshot = File.ReadAllBytes(template.ScreenshotFilePath);
                    template.ProjectFilePath = Path.GetFullPath(Path.Combine(template.TemplatePath, template.ProjectFile)) ;
                    
                    _projectTemplates.Add(template);
                }
                ValidateProjectPath();
            }
            catch(Exception ex)
            {
                Debug.WriteLine(ex.Message);
                Logger.Log(MessageType.Error, "Failed to read project templates");
                throw;
            }
        }
    }
    
}
