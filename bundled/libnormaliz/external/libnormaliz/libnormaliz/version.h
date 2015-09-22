#ifndef NMZ_VERSION_H
#define NMZ_VERSION_H

#define NMZ_VERSION_MAJOR  2
#define NMZ_VERSION_MINOR  99
#define NMZ_VERSION_PATCH  4
#define NMZ_VERSION        2.99.4
#define NMZ_RELEASE (NMZ_VERSION_MAJOR * 10000 + NMZ_VERSION_MINOR * 100 + NMZ_VERSION_PATCH)

namespace libnormaliz {
inline unsigned int getVersion()
{
    return NMZ_RELEASE;
}

} //end namespace libnormaliz

#endif // NMZ_VERSION_H
