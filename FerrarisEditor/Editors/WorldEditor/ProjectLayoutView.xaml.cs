using FerrarisEditor.Components;
using FerrarisEditor.GameProject;
using FerrarisEditor.Utilities;
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
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace FerrarisEditor.Editors
{
    /// <summary>
    /// ProjectLayoutView.xaml 的交互逻辑
    /// </summary>
    public partial class ProjectLayoutView : UserControl
    {
        public ProjectLayoutView()
        {
            InitializeComponent();
        }

        private void OnAddGameEntity_Button_Click(object sender, RoutedEventArgs e)
        {
            // get the datacontext and create the game entity
            var btn = sender as Button;
            var vm = btn.DataContext as Scene;
            vm.AddGameEntityCommand.Execute(new GameEntity(vm) { Name = "Empty Game Entity"});
        }

        private void OnGameEntities_ListBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var listBox = sender as ListBox;
            var newSelection = listBox.SelectedItems.Cast<GameEntity>().ToList();
            var previousSelection = newSelection.Except(e.AddedItems.Cast<GameEntity>()).Concat(e.RemovedItems.Cast<GameEntity>()).ToList();

            Project.UndoRedo.Add(new UndoRedoAction(
                () =>// undo action
                {
                    listBox.UnselectAll();
                    // select the previous selection
                    previousSelection.ForEach(x => (listBox.ItemContainerGenerator.ContainerFromItem(x) as ListBoxItem).IsSelected = true);
                },
                () => //redo action
                {
                    listBox.UnselectAll();
                    newSelection.ForEach(x => (listBox.ItemContainerGenerator.ContainerFromItem(x) as ListBoxItem).IsSelected = true);
                },
                "Selection changed"
                ));
            MSGameEntity msEntity = null;
            if(newSelection.Any())// exist mult-selection
            {
                msEntity = new MSGameEntity(newSelection);// create a MSGameEntity
            }
            GameEntityView.Instance.DataContext = msEntity;// one not selection entity, here is null
        }
    }
}
