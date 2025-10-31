#include <gtest/gtest.h>
#include "ResolverEngine.h"

TEST(ResolverEngineTest, BasicSanity) 
{
    ResolverEngine engine;
    EXPECT_TRUE(engine.isValid());
}
