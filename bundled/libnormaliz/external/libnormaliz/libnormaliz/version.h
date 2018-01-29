#ifndef NMZ_VERSION_H
#define NMZ_VERSION_H

#define NMZ_VERSION_MAJOR  3
#define NMZ_VERSION_MINOR  5
#define NMZ_VERSION_PATCH  0
#define NMZ_VERSION        3.5.0
#define NMZ_RELEASE (NMZ_VERSION_MAJOR * 10000 + NMZ_VERSION_MINOR * 100 + NMZ_VERSION_PATCH)

namespace libnormaliz {
inline unsigned int getVersion()
{
    return NMZ_RELEASE;
}

} //end namespace libnormaliz

#endif // NMZ_VERSION_H
