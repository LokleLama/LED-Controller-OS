#include <gtest/gtest.h>
#include "VariableStore/VariableStore.h"

class VariableStoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        store = &VariableStore::getInstance();
    }

    VariableStore *store;
};

// ---------------------------------------------------------------------------
//  Singleton
// ---------------------------------------------------------------------------

TEST_F(VariableStoreTest, SingletonReturnsSameInstance) {
    auto &a = VariableStore::getInstance();
    auto &b = VariableStore::getInstance();
    EXPECT_EQ(&a, &b);
}

// ---------------------------------------------------------------------------
//  Add and Get variables
// ---------------------------------------------------------------------------

TEST_F(VariableStoreTest, AddStringVariable) {
    auto var = store->addVariable("test.str", "hello");
    ASSERT_NE(var, nullptr);
    EXPECT_EQ(var->asString(), "hello");
    EXPECT_EQ(var->getType(), IVariable::Type::STRING);
}

TEST_F(VariableStoreTest, AddIntVariable) {
    auto var = store->addVariable("test.int", 42);
    ASSERT_NE(var, nullptr);
    EXPECT_EQ(var->asInt(), 42);
    EXPECT_EQ(var->getType(), IVariable::Type::INT);
}

TEST_F(VariableStoreTest, AddFloatVariable) {
    auto var = store->addVariable("test.float", 3.14f);
    ASSERT_NE(var, nullptr);
    EXPECT_FLOAT_EQ(var->asFloat(), 3.14f);
    EXPECT_EQ(var->getType(), IVariable::Type::FLOAT);
}

TEST_F(VariableStoreTest, AddBoolVariable) {
    auto var = store->addBoolVariable("test.bool", true);
    ASSERT_NE(var, nullptr);
    EXPECT_TRUE(var->asBool());
    EXPECT_EQ(var->getType(), IVariable::Type::BOOL);
}

// ---------------------------------------------------------------------------
//  Set and type conversion
// ---------------------------------------------------------------------------

TEST_F(VariableStoreTest, SetStringVariable) {
    store->addVariable("test.set", "initial");
    EXPECT_TRUE(store->setVariable("test.set", "updated"));
    auto var = store->getVariable("test.set");
    ASSERT_NE(var, nullptr);
    EXPECT_EQ(var->asString(), "updated");
}

TEST_F(VariableStoreTest, IntToStringConversion) {
    auto var = store->addVariable("test.conv", 123);
    ASSERT_NE(var, nullptr);
    EXPECT_EQ(var->asString(), "123");
}

TEST_F(VariableStoreTest, BoolToIntConversion) {
    auto var = store->addBoolVariable("test.b2i", true);
    ASSERT_NE(var, nullptr);
    EXPECT_EQ(var->asInt(), 1);
}

// ---------------------------------------------------------------------------
//  Get non-existent variable
// ---------------------------------------------------------------------------

TEST_F(VariableStoreTest, GetNonExistentReturnsNull) {
    auto var = store->getVariable("does.not.exist.ever");
    EXPECT_EQ(var, nullptr);
}

// ---------------------------------------------------------------------------
//  Callbacks
// ---------------------------------------------------------------------------

TEST_F(VariableStoreTest, CallbackFiresOnChange) {
    store->addVariable("test.cb", "old");
    bool fired = false;
    std::string received_val;
    store->registerCallback("test.cb", [&](const std::string &, const std::string &val) {
        fired = true;
        received_val = val;
        return true;
    });
    store->setVariable("test.cb", "new");
    EXPECT_TRUE(fired);
    EXPECT_EQ(received_val, "new");
}

// ---------------------------------------------------------------------------
//  Variable substitution
// ---------------------------------------------------------------------------

TEST_F(VariableStoreTest, FindAndReplaceVariables) {
    store->addVariable("greeting", "world");
    std::string result = store->findAndReplaceVariables("hello ${greeting}!");
    EXPECT_NE(result.find("world"), std::string::npos);
}

TEST_F(VariableStoreTest, FindAndReplaceNoMatch) {
    std::string input = "no variables here";
    std::string result = store->findAndReplaceVariables(input);
    EXPECT_EQ(result, input);
}

// ---------------------------------------------------------------------------
//  GetAllVariables
// ---------------------------------------------------------------------------

TEST_F(VariableStoreTest, GetAllVariablesContainsAdded) {
    store->addVariable("test.all", "value");
    auto all = store->getAllVariables();
    EXPECT_NE(all.find("test.all"), all.end());
    EXPECT_EQ(all["test.all"], "value");
}
