using FerrarisEditor.GameProject;
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

namespace FerrarisEditor.Components
{
    [DataContract]
    [KnownType(typeof(Transform))]
    class GameEntity : ViewModelBase
    {
        public bool _isEnabled = true;
        [DataMember]
        public bool IsEnabled
        {
            get => _isEnabled;
            set
            {
                if(_isEnabled != value)
                {
                    _isEnabled = value;
                    OnPropertyChanged(nameof(IsEnabled));
                }
            }
        }

        private string _name;

        [DataMember]
        public string Name
        {
            get => _name;

            set
            {
                if(_name !=value)
                {
                    _name = value;
                    OnPropertyChanged(nameof(Name));
                }
            }
        }
        [DataMember]
        public Scene ParentScene { get; private set; }

        [DataMember(Name =nameof(Components))]
        private readonly ObservableCollection<Component> _components = new ObservableCollection<Component>();
        public ReadOnlyObservableCollection<Component> Components { get; private set; }

        public ICommand RenameCommand { get; private set; }
        public ICommand IsEnabledCommand { get; private set; }

        [OnDeserialized]
        void OnDeserialized(StreamingContext context)
        {
            if(_components!=null)
            {
                Components = new ReadOnlyObservableCollection<Component>(_components);
                OnPropertyChanged(nameof(Components));
            }

            RenameCommand = new RelayCommand<string>(x=>
            {
                var oldName = Name;
                Name = x;

                Project.UndoRedo.Add(new UndoRedoAction(nameof(Name), this, oldName, x,
                    $"Rename entity '{oldName}' to '{x}'"));
            }, x => x != _name);// oldname not equal new name

            IsEnabledCommand = new RelayCommand<bool>(x =>
            {
                var oldValue = _isEnabled;
                IsEnabled = x;

                Project.UndoRedo.Add(new UndoRedoAction(nameof(IsEnabled), this,
                    oldValue, x, x? $"Enable {Name}" :  $"Disable{Name}"));
            });
        }

        public GameEntity(Scene scene)
        {
            Debug.Assert(scene != null);
            ParentScene = scene;
            _components.Add(new Transform(this));
            OnDeserialized(new StreamingContext());
        }

    }

    abstract class MSEntity : ViewModelBase
    {
        // Enable update to select entities
        private bool _enableUpdate = true;
        public bool? _isEnabled = true;
        public bool? IsEnabled
        {
            get => _isEnabled;
            set
            {
                if (_isEnabled != value)
                {
                    _isEnabled = value;
                    OnPropertyChanged(nameof(IsEnabled));
                }
            }
        }
        private string _name;// name can null, do not need to nullable
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

        private readonly ObservableCollection<IMSComponent> _components = new ObservableCollection<IMSComponent>();
        public ReadOnlyObservableCollection<IMSComponent> Components { get; }

        public List<GameEntity> SelectedEntities { get; }
        
        /**
         * Following three method to get property from Entities
         */
        public static float? GetMixedValues(List<GameEntity> entities, Func<GameEntity, float> getProperty)
        {
            var value = getProperty(entities.First());
            foreach(var entity in entities.Skip(1))// all are same, return value, else return null
            {
                if(!value.IsTheSameAs(getProperty(entity)))
                {
                    return null;
                }
            }
            return value;
        }

        public static bool? GetMixedValue(List<GameEntity> entities, Func<GameEntity,bool> getProperty)
        {
            var value = getProperty(entities.First());
            foreach (var entity in entities.Skip(1))// all are same, return value, else return null
            {
                if(value != getProperty(entity))
                {
                    return null;
                }
            }
            return value;
        }
        public static string GetMixedValue(List<GameEntity> entities, Func<GameEntity, string> getProperty)
        {
            var value = getProperty(entities.First());
            foreach (var entity in entities.Skip(1))// all are same, return value, else return null
            {
                if (value != getProperty(entity))
                {
                    return null;
                }
            }
            return value;
        }


        public void Refresh()
        {
            _enableUpdate = false;
            UpdateMSGameEntitiy();// momo007 later rename the method
            _enableUpdate = true;
        }

        /// <summary>
        /// Updates the Property by selected entity
        /// </summary>
        /// <returns></returns>
        protected virtual bool UpdateMSGameEntitiy()
        {
            // Func: the function to get property from entity, details see GetMixedValue
            IsEnabled = GetMixedValue(SelectedEntities, new Func<GameEntity, bool>(x => x.IsEnabled));
            Name = GetMixedValue(SelectedEntities, new Func<GameEntity, string>(x => x.Name));

            return true;
        }

        /// <summary>
        /// Update the entity by the Property
        /// </summary>
        /// <param name="propertyName">Name of the update property.</param>
        /// <returns></returns>
        protected virtual bool UpdateGameEntities(string propertyName)
        {
            switch (propertyName)
            {
                // iterative each entity to update property
                case nameof(IsEnabled): SelectedEntities.ForEach(x => x.IsEnabled = IsEnabled.Value); return true;
                case nameof(Name): SelectedEntities.ForEach(x => x.Name = Name); return true;
            }
            return false;
        }


        public MSEntity(List<GameEntity> entities)
        {
            Debug.Assert(entities?.Any() == true);
            Components = new ReadOnlyObservableCollection<IMSComponent>(_components);
            SelectedEntities = entities;
            PropertyChanged += (s, e) => { if(_enableUpdate) UpdateGameEntities(e.PropertyName); };
        }
    }
    class MSGameEntity : MSEntity
    {
        public MSGameEntity(List<GameEntity> entities) : base(entities)
        {
            Refresh();// fecth all the data from selected game entities
        }
    }
}
