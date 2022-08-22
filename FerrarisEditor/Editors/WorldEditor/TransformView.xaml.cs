using FerrarisEditor.Components;
using FerrarisEditor.GameProject;
using FerrarisEditor.Utilities;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;

namespace FerrarisEditor.Editors
{
    /// <summary>
    /// Interaction logic for TransformView.xaml
    /// </summary>
    public partial class TransformView : UserControl
    {
        private Action _undoAction = null;
        private bool _propertyChanged = false;
        public TransformView()
        {
            InitializeComponent();
            Loaded += OnTransformViewLoaded;
        }

        private void OnTransformViewLoaded(object sender, RoutedEventArgs e)
        {
            Loaded -= OnTransformViewLoaded;
            (DataContext as MSTransform).PropertyChanged += (s, e) => _propertyChanged = true;
        }

        private Action GetAction()
        {
            if (!(DataContext is MSTransform vm))
            {
                _undoAction = null;
                _propertyChanged = false;
                return null;
            }
            var selection = vm.SelectedComponents.Select(transform => (transform, transform.Position)).ToList();
            return new Action(() =>
            {
                selection.ForEach(item => item.transform.Position = item.Position);
                (GameEntityView.Instance.DataContext as MSEntity)?.GetMSComponent<MSTransform>().Refresh();
            });
        }

        private void OnPosition_VectorBox_PreviewMouse_LBD(object sender, MouseButtonEventArgs e)
        {
            _propertyChanged = false;
            _undoAction = GetAction();
        }

        private void OnPosition_VectorBox_PreviewMouse_LBU(object sender, MouseButtonEventArgs e)
        {
            if (_propertyChanged)
            {
                Debug.Assert(_undoAction != null);
                _propertyChanged = false;
                var redoAction = GetAction();
                Project.UndoRedo.Add(new UndoRedoAction(_undoAction, redoAction, "Position changed"));
            }

        }

        private void OnPosition_VectorBox_LostKeyboardFocus(object sender, KeyboardFocusChangedEventArgs e)
        {
            if(_propertyChanged && _undoAction != null)
            {
                OnPosition_VectorBox_PreviewMouse_LBU(sender, null);
            }
        }
    }
}
