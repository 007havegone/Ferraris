using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace FerrarisEditor.Dictionaries
{
    public partial class ControlTemplates : ResourceDictionary
    {
        private void OnTextBox_KeyDown(object sender, System.Windows.Input.KeyEventArgs e)
        {
            // check for commands and keys
            var textBox = sender as TextBox;
            var exp = textBox.GetBindingExpression(TextBox.TextProperty);
            if (exp == null) return;
            if(e.Key == Key.Enter)
            {
                // command Tag is well define
                if(textBox.Tag is ICommand command && command.CanExecute(textBox.Text))
                {
                    command.Execute(textBox.Text);// call command
                }
                else
                {
                    exp.UpdateSource();// only update without command
                }
                Keyboard.ClearFocus();
                e.Handled = true;
            }
            else if(e.Key == Key.Escape)
            {
                exp.UpdateTarget();// rockback to the oldvalue
                Keyboard.ClearFocus();
            }

        }

        private void OnTextBoxRename_KeyDown(object sender, KeyEventArgs e)
        {
            // check for commands and keys
            var textBox = sender as TextBox;
            var exp = textBox.GetBindingExpression(TextBox.TextProperty);
            if (exp == null) return;
            if (e.Key == Key.Enter)
            {
                // command Tag is well define
                if (textBox.Tag is ICommand command && command.CanExecute(textBox.Text))
                {
                    command.Execute(textBox.Text);// call command
                }
                else
                {
                    exp.UpdateSource();// only update without command
                }
                textBox.Visibility = Visibility.Collapsed;
                e.Handled = true;
            }
            else if (e.Key == Key.Escape)
            {
                exp.UpdateTarget();// rockback to the oldvalue
                textBox.Visibility = Visibility.Collapsed;
            }
        }

        private void OnTextBoxRename_LostFocus(object sender, RoutedEventArgs e)
        {
            var textBox = sender as TextBox;
            var exp = textBox.GetBindingExpression(TextBox.TextProperty);
            if(exp != null)
            {
                exp.UpdateTarget();
                textBox.MoveFocus(new TraversalRequest(FocusNavigationDirection.Previous));
                textBox.Visibility = Visibility.Collapsed;
            }
        }
        private void OnClock_Button_Clcik(object sender, RoutedEventArgs e)
        {
            var window = (Window)((FrameworkElement)sender).TemplatedParent;
            window.Close();
        }

        private void OnMaximizeRestore_Button_Click(object sender, RoutedEventArgs e)
        {
            var window = (Window)((FrameworkElement)sender).TemplatedParent;
            window.WindowState = (window.WindowState == WindowState.Normal) ?
                WindowState.Maximized : WindowState.Normal;
        }

        private void OnMinimize_Button_Click(object sender, RoutedEventArgs e)
        {
            var window = (Window)((FrameworkElement)sender).TemplatedParent;
            window.WindowState = WindowState.Minimized;
        }

        
    }
}
