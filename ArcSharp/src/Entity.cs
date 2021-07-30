using System;

namespace ArcEngine
{
    public class Entity
    {
        public uint SceneID { get; private set; }
        public uint EntityID { get; private set; }

        ~Entity()
        {
            Console.WriteLine("Destroyed Entity {0}:{1}", SceneID, EntityID);
        }
    }
}
