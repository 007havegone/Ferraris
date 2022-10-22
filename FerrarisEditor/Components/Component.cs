using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;

namespace FerrarisEditor.Components
{
    interface IMSComponent { }

    [DataContract]
    abstract class Component:ViewModelBase
    {
        [DataMember]
        public GameEntity Owner { get; private set; }
        public abstract IMSComponent GetMultiselectionComponent(MSEntity mSEntity);
        public abstract void WriteToBinary(BinaryWriter bw);
        public Component(GameEntity owner)
        {
            Debug.Assert(owner != null);
            Owner = owner;
        }
    }
    // the basic MScomponent class for providing the interface.
    abstract class MSComponent<T> : ViewModelBase, IMSComponent where T : Component
    {
        private bool _enableUpdates = true;// prevent the cyclic update
        public List<T> SelectedComponents { get; }

        protected abstract bool UpdateComponents(string propertyName);

        protected abstract bool UpdateMSComponent();

        public void Refresh()
        {
            _enableUpdates = false;
            UpdateMSComponent();
            _enableUpdates = true;
        }

        public MSComponent(MSEntity mSEntity)
        {
            Debug.Assert(mSEntity?.SelectedEntities?.Any() == true);
            // get the component from the selected entity
            SelectedComponents = mSEntity.SelectedEntities.Select(entity => entity.GetComponent<T>()).ToList();
            // Update the components
            PropertyChanged += (s, e) => { if (_enableUpdates) UpdateComponents(e.PropertyName); };
        }
    }
}
