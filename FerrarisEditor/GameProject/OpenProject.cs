using FerrarisEditor.Utilities;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;

namespace FerrarisEditor.GameProject
{
    /// <summary>
    /// 存储打开项目的数据
    /// </summary>
    [DataContract]
    public class ProjectData
    {
        [DataMember]
        public string ProjectName { get; set; }
        [DataMember]
        public string ProjectPath { get; set; }
        [DataMember]
        public DateTime Date { get; set; }

        public string FullPath { get => $"{ProjectPath}{ProjectName}{Project.Extension}"; }

        public byte[] Icon { get; set; }
        public byte[] Screenshot { get; set; }
    }
    [DataContract]
    public class ProjectDataList
    {
        [DataMember]
        public List<ProjectData> Projects { get; set; }
    }
    class OpenProject
    {
        // the path save the DataList which contain the opened project data.
        private static readonly string _applicatiopnDataPath = $@"{Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData)}\FerrarisEditor\";
        private static readonly string _projectDataPath;
        // bind to my control
        private static readonly ObservableCollection<ProjectData> _projects = new ObservableCollection<ProjectData>();

        public static ReadOnlyObservableCollection<ProjectData> Projects
        { get; }

        static OpenProject()
        {

            try
            {
                // 查看存储project相关信息的文件夹是否存在，不存在创建
                if (!Directory.Exists(_applicatiopnDataPath)) Directory.CreateDirectory(_applicatiopnDataPath);
                _projectDataPath = $@"{_applicatiopnDataPath}ProjectData.xml";// 项目元数据

                Projects = new ReadOnlyObservableCollection<ProjectData>(_projects);
                ReadProjectData();
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
                // TODO: log error
            }
        }

        private static void WritePorjectData()
        {
            var project = _projects.OrderBy(x => x.Date).ToList();
            Serializer.ToFile(new ProjectDataList() { Projects = project }, _projectDataPath);// update the serializer file
        }


        public static Project Open(ProjectData data)
        {
            ReadProjectData();
            var project = _projects.FirstOrDefault(x => x.FullPath == data.FullPath);
            if (project != null)// exist, update the last open time
            {
                project.Date = DateTime.Now;
            }
            else// not exist, add to list
            {
                project = data;
                project.Date = DateTime.Now;
                _projects.Add(project);
            }
            WritePorjectData();

            return Project.Load(project.FullPath);
        }

        
       

        private static void ReadProjectData()
        {
            if(File.Exists(_projectDataPath))
            {
                var projects = Serializer.FromFile<ProjectDataList>(_projectDataPath).Projects.OrderByDescending(x => x.Date);
                _projects.Clear();
                foreach(var project in projects)
                {
                    if(File.Exists(project.FullPath))// do not contain projects which are remove
                    {
                        project.Icon = File.ReadAllBytes($@"{project.ProjectPath}\.Ferraris\icon.png");
                        project.Screenshot = File.ReadAllBytes($@"{project.ProjectPath}\.Ferraris\Screenshot.png");
                        _projects.Add(project);
                    }
                }
            }
        }
    }
}
