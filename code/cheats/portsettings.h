#ifndef PORTSETTINGS_H
#define PORTSETTINGS_H

namespace PortSettings
{
    bool NoTimers();
    void SetNoTimers( bool enabled );

    bool MapOnTop();
    void SetMapOnTop( bool onTop );

    void Save();
}
#endif
