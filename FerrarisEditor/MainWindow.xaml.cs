using FerrarisEditor.GameProject;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;

namespace FerrarisEditor
{
    /// <summary>
    /// MainWindow.xaml 的交互逻辑
    /// </summary>
    public partial class MainWindow : Window
    {
        public static string FerrarisPath { get; private set; } //  = @"F:\Source\Repos\Ferraris";

        public MainWindow()
        {
            InitializeComponent();
            Loaded += OnMainWindowLoaded;
            Closing += OnMainWindowClosing;
        }
        private void OnMainWindowLoaded(object sender, RoutedEventArgs e)
        {
            Loaded -= OnMainWindowLoaded;
            GetEnginePath();
            OpenProjectBrowserDialog();
        }

        private void GetEnginePath()
        {
            // try to get USER enviroment variable
            var ferrarisPath = Environment.GetEnvironmentVariable("FERRARIS_ENGINE", EnvironmentVariableTarget.User);
            if (ferrarisPath == null || !Directory.Exists(Path.Combine(ferrarisPath,@"Engine\EngineAPI")))
            {
                var dlg = new EnginePathDialog();// show the dlg to set the Engine path
                if(dlg.ShowDialog() == true)
                {
                    FerrarisPath = dlg.FerrarisPath;
                    Environment.SetEnvironmentVariable("FERRARIS_ENGINE", FerrarisPath.ToUpper(), EnvironmentVariableTarget.User);// set the user enviroment variable
                }
                else
                {
                    Application.Current.Shutdown();
                }
            }
            else
            {
                FerrarisPath = ferrarisPath;
            }
        }

        private void OnMainWindowClosing(object sender, CancelEventArgs e)
        {
            Closing -= OnMainWindowClosing;
            Project.Current?.Unload();
        }


        private void OpenProjectBrowserDialog()
        {
            var projectBrowser = new ProjectBrowserDialog();
            if(projectBrowser.ShowDialog() == false || projectBrowser.DataContext == null)// check the dialog
            {
                Application.Current.Shutdown();
            }
            else
            {
                Project.Current?.Unload();// if had project loaded, unload it.
                DataContext = projectBrowser.DataContext;// pass the dialog data context to Main window
            }
        }
    }
}
