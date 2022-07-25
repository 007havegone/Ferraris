using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Data;

namespace FerrarisEditor.Utilities
{
    enum MessageType
    {
        Info = 0x01,
        Warning = 0x02,
        Error = 0x04,
    }
    class LoggerMessage
    {
        public DateTime Time { get; }
        public MessageType MessageType { get; }
        public string Message { get; }
        public string File { get; }
        public string Caller { get; }
        public int Line { get; }
        public string MetaData => $"{File}:{Caller}({Line})";


        public LoggerMessage(MessageType type, string msg, string file, string caller, int line)
        {
            Time = DateTime.Now;
            MessageType = type;
            Message = msg;
            File = Path.GetFileName(file);
            Caller = caller;
            Line = line;
        }

    }
    static class Logger
    {
        private static int _messageFilter = (int)(MessageType.Info | MessageType.Error | MessageType.Warning);
        private static readonly ObservableCollection<LoggerMessage> _message = new ObservableCollection<LoggerMessage>();
        public static ReadOnlyObservableCollection<LoggerMessage> Messages
        { get; } = new ReadOnlyObservableCollection<LoggerMessage>(_message);
        public static CollectionViewSource FilteredMessages
        { get; } = new CollectionViewSource() { Source = Messages };

        // method record messasge support multi-thread
        public static async void Log(MessageType type, string msg,
            [CallerFilePath] string file = "", [CallerMemberName] string caller = "",
            [CallerLineNumber] int line = 0)
        {
            await Application.Current.Dispatcher.BeginInvoke(new Action(() =>
            {
                _message.Add(new LoggerMessage(type, msg, file, caller, line));
            }));
        }

        public static async void Clear()
        {
            await Application.Current.Dispatcher.BeginInvoke(new Action(() =>
            {
                _message.Clear();
            }));
        }

        public static void SetMessageFilter(int mask)
        {
            _messageFilter = mask;
            FilteredMessages.View.Refresh();
        }

        static Logger()
        {
            FilteredMessages.Filter += (s, e) =>
            {
                var type = (int)(e.Item as LoggerMessage).MessageType;
                e.Accepted = (type & _messageFilter) != 0;
            };
        }
    }
}
