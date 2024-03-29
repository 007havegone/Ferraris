﻿using FerrarisEditor.Components;
using FerrarisEditor.Utilities;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;

namespace FerrarisEditor.GameProject
{
    [DataContract]
    class Scene : ViewModelBase
    {
        private string _name;// scene name
        [DataMember]
        public string Name
        {
            get => _name;
            set
            {
                if(_name != value)
                {
                    _name = value;
                    OnPropertyChanged(nameof(Name));
                }
            }
        }

        [DataMember]
        public Project Project { get; private set; } // easy to know which Project is ref to

        // use Project get property of active
        private bool _isActive;

        [DataMember]
        public bool IsActive
        {
            get => _isActive;

            set
            {
                if(_isActive != value)
                {
                    _isActive = value;
                    OnPropertyChanged(nameof(IsActive));
                }
            }
        } 
        [DataMember(Name = nameof(GameEntities))]
        private ObservableCollection<GameEntity> _gameEntities = new ObservableCollection<GameEntity>();
        public ReadOnlyObservableCollection<GameEntity> GameEntities { get; private set; }

        public ICommand AddGameEntityCommand { get; private set; }
        public ICommand RemoveGameEntityCommand { get; private set; }
        private void AddGameEnitity(GameEntity entity, int index = -1)
        {
            Debug.Assert(!_gameEntities.Contains(entity));
            entity.IsActive = IsActive;
            if(index == -1)// add new entity
            {
                _gameEntities.Add(entity);
            }
            else// undo action insert entity
            {
                _gameEntities.Insert(index, entity);
            }
        }
        private void RemoveGameEnitity(GameEntity entity)
        {
            Debug.Assert(_gameEntities.Contains(entity));
            entity.IsActive = false;
            _gameEntities.Remove(entity);
        }
        [OnDeserialized]// call this function after serialized done
        private void OnDeserialized(StreamingContext context)
        {
            if (_gameEntities != null)
            {
                GameEntities = new ReadOnlyObservableCollection<GameEntity>(_gameEntities);
                OnPropertyChanged(nameof(GameEntities));
            }
            foreach (var entity in _gameEntities)
            {
                entity.IsActive = IsActive;
            }

            // create the command
            AddGameEntityCommand = new RelayCommand<GameEntity>(x =>
            {
                AddGameEnitity(x);
                var entityIndex = _gameEntities.Count - 1;// insert into last place, why need index?
                Project.UndoRedo.Add(new UndoRedoAction(
                    () => RemoveGameEnitity(x),
                    () => AddGameEnitity(x, entityIndex),
                    $"Add {x.Name} to {Name}")) ;
            });

            RemoveGameEntityCommand = new RelayCommand<GameEntity>(x =>
            {
                var entityIndex = _gameEntities.IndexOf(x);
                RemoveGameEnitity(x);

                Project.UndoRedo.Add(new UndoRedoAction(
                    () => AddGameEnitity(x, entityIndex),
                    () => RemoveGameEnitity(x),
                    $"Remove {x.Name}"));
            });
        }


        public Scene(Project project, string name)
        {
            Debug.Assert(project != null);
            Project = project;
            Name = name;
            OnDeserialized(new StreamingContext());
        }
    }
}
