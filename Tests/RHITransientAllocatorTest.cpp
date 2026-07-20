// ToyEngine - RHI 瞬态范围分配器回归测试

#include "RHITransientAllocator.h"

#include <cstdint>
#include <iostream>
#include <limits>

namespace {

[[nodiscard]] bool Expect(const bool condition, const char* const message)
{
    if (!condition)
    {
        std::cerr << "[FAIL] " << message << '\n';
        return false;
    }
    return true;
}

[[nodiscard]] bool TestInitializationValidation()
{
    TE::RHITransientRangeAllocator allocator;
    return Expect(!allocator.Initialize(0, 2, 256), "zero bytes per frame must fail") &&
           Expect(!allocator.Initialize(1024, 0, 256), "zero frame count must fail") &&
           Expect(!allocator.Initialize(1024, 2, 0), "zero alignment must fail") &&
           Expect(!allocator.Initialize(std::numeric_limits<uint64_t>::max(), 2, 256),
                  "total-size overflow must fail");
}

[[nodiscard]] bool TestAlignmentAndFrameIsolation()
{
    TE::RHITransientRangeAllocator allocator;
    if (!Expect(allocator.Initialize(1024, 2, 256), "valid allocator initialization") ||
        !Expect(allocator.GetTotalSize() == 2048, "total ring size"))
    {
        return false;
    }

    uint64_t offset = 0;
    allocator.BeginFrame(0);
    if (!Expect(allocator.Allocate(1, offset) && offset == 0, "first frame starts at zero") ||
        !Expect(allocator.Allocate(16, offset) && offset == 256, "allocation obeys alignment") ||
        !Expect(allocator.Allocate(512, offset) && offset == 512, "allocation reaches frame segment end") ||
        !Expect(!allocator.Allocate(1, offset), "frame segment overflow must fail"))
    {
        return false;
    }

    allocator.BeginFrame(1);
    if (!Expect(allocator.Allocate(64, offset) && offset == 1024, "second frame uses isolated segment"))
    {
        return false;
    }

    allocator.BeginFrame(2);
    return Expect(allocator.Allocate(64, offset) && offset == 0,
                  "wrapped frame index resets its own segment");
}

} // namespace

int main()
{
    std::cout << "[RHITransientAllocatorTest] validating transient frame segments...\n";
    if (!TestInitializationValidation() || !TestAlignmentAndFrameIsolation())
    {
        return 1;
    }

    std::cout << "[RHITransientAllocatorTest] all passed.\n";
    return 0;
}
