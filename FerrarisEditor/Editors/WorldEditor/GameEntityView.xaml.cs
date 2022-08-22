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
    /// GameEntityView.xaml 的交互逻辑
    /// </summary>
    public partial class GameEntityView : UserControl
    {
        private Action _undoAction;
        private string _propertyName;
        public static GameEntityView Instance { get; private set; }
        public GameEntityView()
        {
            InitializeComponent();
            DataContext = null;
            Instance = this;
            DataContextChanged += (_, __) =>
            {
                if(DataContext!=null)
                {
                    // record the changed name
                    (DataContext as MSEntity).PropertyChanged += (s, e) => _propertyName = e.PropertyName;
                }
            };
        }
        private Action GetRenameAction()
        {
            // remember all selection name and restore the mixed value
            var vm = DataContext as MSEntity;
            var selection = vm.SelectedEntities.Select(entity => (entity, entity.Name)).ToList();
            // use the entities changed name to create the Undo/Redo Action
           return new Action(() =>
            {
                selection.ForEach(item => item.entity.Name = item.Name);//get the name from selection enetity
                (DataContext as MSEntity).Refresh();// and store their mixed value
            });
        }

        private Action GetIsEnabledAction()
        {
            // remember all selection name and restore the mixed value
            var vm = DataContext as MSEntity;
            var selection = vm.SelectedEntities.Select(entity => (entity, entity.IsEnabled)).ToList();
            // use the entities changed name to create the Undo/Redo Action
            return new Action(() =>
            {
                selection.ForEach(item => item.entity.IsEnabled = item.IsEnabled);//get the name from selection enetity
                (DataContext as MSEntity).Refresh();// and store their mixed value
            });
        }

        private void OnName_TextBox_GotKeyBoardFocus(object sender, KeyboardFocusChangedEventArgs e)
        {
            // use the entities changed name to create the undoAction
            _propertyName = string.Empty;
            _undoAction = GetRenameAction();
        }

        private void OnName_TextBox_LostKeyBoardFocus(object sender, KeyboardFocusChangedEventArgs e)
        {
            if(_propertyName == nameof(MSEntity.Name) && _undoAction != null)
            {
                var redoAction = GetRenameAction();
                Project.UndoRedo.Add(new UndoRedoAction(_undoAction, redoAction, "Rename game entity"));// add command
                _propertyName = null;
            }
            _undoAction = null;// reset the undoAction
        }

        private void OnIsEnabled_CkeckBox_Click(object sender, RoutedEventArgs e)
        {
            var undoAction = GetIsEnabledAction();
            var vm = DataContext as MSEntity;
            vm.IsEnabled = (sender as CheckBox).IsChecked == true;
            var redoAction = GetIsEnabledAction();
            Project.UndoRedo.Add(new UndoRedoAction(undoAction, redoAction,
                vm.IsEnabled == true ? "Enable game entity" : "Disable game entity"));
        }
    }
}
