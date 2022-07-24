using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace FerrarisEditor.Utilities
{

    public interface IUndoRedo
    {
        string Name { get; }
        void Undo();
        void Redo();
    }

    public class UndoRedoAction : IUndoRedo
    {
        private Action _undoAction;
        private Action _redoAction;
        public string Name { get; }

        public void Redo() => _redoAction();

        public void Undo() => _undoAction();

        public UndoRedoAction(string name)
        {
            Name = name;
        }

        public UndoRedoAction(Action undo, Action redo, string name):this(name)
        {
            Debug.Assert(undo != null && redo != null);
            _undoAction = undo;
            _redoAction = redo;
        }
        /// <summary>
        /// Initializes a new instance of the <see cref="UndoRedoAction"/> class.
        /// </summary>
        /// <param name="property">The property in GameEntity</param>
        /// <param name="instance">instance of GameEntity</param>
        /// <param name="undoValue">The old value.</param>
        /// <param name="redoValue">The new value.</param>
        /// <param name="name">The Action name</param>
        public UndoRedoAction(string property, object instance, object undoValue, object redoValue, string name)
            : this(
                () => instance.GetType().GetProperty(property).SetValue(instance, undoValue),// use reflection to get the type of GameEntity
                () => instance.GetType().GetProperty(property).SetValue(instance, redoValue),
                name)
        { }
    }
        


    public class UndoRedo
    {
        private bool _enableAdd = true;
        private readonly ObservableCollection<IUndoRedo> _redoList = new ObservableCollection<IUndoRedo>();
        private readonly ObservableCollection<IUndoRedo> _undoList = new ObservableCollection<IUndoRedo>();

        public ReadOnlyObservableCollection<IUndoRedo> RedoList { get; }
        public ReadOnlyObservableCollection<IUndoRedo> UndoList { get; }

        public void Reset()
        {
            _redoList.Clear();
            _undoList.Clear();
        }

        public void Add(IUndoRedo cmd)
        {
            if(_enableAdd)
            {
                _undoList.Add(cmd);
                _redoList.Clear();// once we have a new cmd, the redoList must empty
            }
        }

        public void Undo()
        {
            if(_undoList.Any())
            {
                var cmd = _undoList.Last();
                _undoList.RemoveAt(_undoList.Count - 1);
                _enableAdd = false;
                cmd.Undo();
                _enableAdd = true;
                _redoList.Insert(0, cmd);
            }
        }

        public void Redo()
        {
            if(_redoList.Any())
            {
                var cmd = _redoList.First();
                _redoList.RemoveAt(0);
                _enableAdd = false;
                cmd.Redo();
                _enableAdd = true;
                _undoList.Add(cmd);
            }
        }

        public UndoRedo()
        {
            RedoList = new ReadOnlyObservableCollection<IUndoRedo>(_redoList);
            UndoList = new ReadOnlyObservableCollection<IUndoRedo>(_undoList);
        }
    }
}
