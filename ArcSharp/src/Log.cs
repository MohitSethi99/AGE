using System.Runtime.CompilerServices;

namespace ArcEngine
{
	public class Log
	{
		public static void Critical(string message)
		{
			Fatal_Critical(message);
		}
		[MethodImpl(MethodImplOptions.InternalCall)]
		private static extern void Fatal_Critical(string message);

		public static void Error(string message)
		{
			Error_Native(message);
		}
		[MethodImpl(MethodImplOptions.InternalCall)]
		private static extern void Error_Native(string message);

		public static void Warn(string message)
		{
			Warn_Native(message);
		}
		[MethodImpl(MethodImplOptions.InternalCall)]
		private static extern void Warn_Native(string message);

		public static void Info(string message)
		{
			Info_Native(message);
		}
		[MethodImpl(MethodImplOptions.InternalCall)]
		private static extern void Info_Native(string message);

		public static void Trace(string message)
		{
			Trace_Native(message);
		}
		[MethodImpl(MethodImplOptions.InternalCall)]
		private static extern void Trace_Native(string message);
	}
}
