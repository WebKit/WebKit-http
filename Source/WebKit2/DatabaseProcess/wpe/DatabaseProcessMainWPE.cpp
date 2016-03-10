#include "config.h"
#include "DatabaseProcessMainUnix.h"

#if ENABLE(DATABASE_PROCESS)

#include "ChildProcessMain.h"
#include "DatabaseProcess.h"

using namespace WebCore;

namespace WebKit {

class DatabaseProcessMain final: public ChildProcessMainBase {
public:
    bool platformInitialize() override
    {
        return true;
    }
};

int DatabaseProcessMainUnix(int argc, char** argv)
{
    return ChildProcessMain<DatabaseProcess, DatabaseProcessMain>(argc, argv);
}

} // namespace WebKit

#endif // ENABLE(DATABASE_PROCESS)
