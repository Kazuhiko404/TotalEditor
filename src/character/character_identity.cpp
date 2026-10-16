#include "common.h"
#include "character_identity.h"
#include "log.h"

#include <kenshi/InstanceID.h>
#include <sstream>
#include <string.h>

namespace totaleditor
{
namespace CharacterIdentity
{
namespace
{
// Frame holds only pointers and PODs, so __try is legal here.
bool ReadEngineIdentity(Character* character, char* uidOut, size_t uidSize, hand* handleOut)
{
    uidOut[0] = '\0';
    TE_TRY
    {
        InstanceID* inst = character->getInstanceID();
        if (inst != NULL && !inst->uid.empty())
        {
            strncpy_s(uidOut, uidSize, inst->uid.c_str(), _TRUNCATE);
        }
        *handleOut = character->getHandle();
        return true;
    }
    TE_CATCH
    {
        return false;
    }
}
} // namespace

bool GetOrCreate(Character* character, std::string& id)
{
    id.clear();
    if (character == NULL) return false;

    char uid[256];
    hand h;
    if (!ReadEngineIdentity(character, uid, sizeof(uid), &h))
        return false;

    if (uid[0] != '\0')
    {
        id = uid;
        return true;
    }

    std::ostringstream ss;
    ss << "char_" << h.container << "_" << h.containerSerial << "_" << h.index << "_" << h.serial;
    id = ss.str();
    return !id.empty();
}

} // namespace CharacterIdentity
} // namespace totaleditor
