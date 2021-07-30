using System;
using ArcEngine;

namespace Example
{
    public class Test : Entity
    {
		public void OnCreate()
		{
			Log.Info("OnCreate!");
		}

		public void OnUpdate(double ts)
		{
			Log.Info("OnUpdate!");
		}
    }
}
