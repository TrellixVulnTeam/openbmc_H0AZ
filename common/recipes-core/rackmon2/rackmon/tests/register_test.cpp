#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "regmap.hpp"

using namespace std;
using namespace testing;

TEST(RegisterTest, BasicCreationCompare) {
  RegisterDescriptor desc{
      0, 2, "HELLO", 1, false, RegisterValueType::STRING, 0};
  Register reg(desc);

  EXPECT_EQ(reg.timestamp, 0);
  EXPECT_EQ(false, reg);
  EXPECT_EQ(reg.value.size(), 2);
  // Set to some known value.
  reg.value = {0x3730, 0x302d};
  reg.timestamp = 0x12345678;
  // bool test should now succeed.
  EXPECT_EQ(true, reg);

  // Conversion to RegisterValue
  RegisterValue sval = reg;
  EXPECT_EQ(sval.type, RegisterValueType::STRING);
  EXPECT_EQ(sval.value.strValue, "700-");

  // Conversion to string.
  std::string strval = reg;
  EXPECT_EQ(strval, "700-");

  Register reg2(desc);
  reg2.value = {0x3730, 0x302d};
  // Even though the regs have same value, since one is not valid,
  // equality should not succeed.
  EXPECT_NE(reg, reg2);

  // Make it valid now, equality should succeed.
  reg2.timestamp = 0x56781234;
  EXPECT_EQ(reg, reg2);

  // Inequality tests.
  reg2.value[1] = 0x3030;
  EXPECT_NE(reg, reg2);
  EXPECT_EQ(reg != reg2, true);
  EXPECT_EQ(reg == reg2, false);
}

TEST(RegisterTest, JSONTest) {
  RegisterDescriptor desc{
      0, 2, "HELLO", 1, false, RegisterValueType::STRING, 0};
  Register reg(desc);

  EXPECT_EQ(reg.timestamp, 0);
  EXPECT_EQ(false, reg);
  EXPECT_EQ(reg.value.size(), 2);
  // Set to some known value.
  reg.value = {0x3730, 0x302d};
  reg.timestamp = 0x12345678;
  nlohmann::json j = reg;
  EXPECT_EQ(j.is_object(), true);
  EXPECT_EQ(j.size(), 2);
  EXPECT_EQ(j["time"], 0x12345678);
  EXPECT_EQ(j["data"], "3730302d");
}

TEST(RegisterStoreTest, BasicOperation) {
  RegisterDescriptor desc{
      0, 2, "HELLO", 5, false, RegisterValueType::STRING, 0};
  RegisterStore reg(desc);
  for (uint16_t i = 0; i < 5; i++) {
    EXPECT_EQ(reg.front(), false);
    reg.front().value = {0x0001, i};
    reg.front().timestamp = i + 1;
    ++reg;
  }
  for (uint16_t i = 0; i < 5; i++) {
    EXPECT_EQ(reg.front(), true);
    EXPECT_EQ(reg.front().value, std::vector<uint16_t>({0x0001, i}));
    EXPECT_EQ(reg.front().timestamp, i + 1);
    ++reg;
    EXPECT_EQ(reg.back(), true);
    EXPECT_EQ(reg.back().value, std::vector<uint16_t>({0x0001, i}));
    EXPECT_EQ(reg.back().timestamp, i + 1);
  }
}

TEST(RegisterStoreTest, DataRetrievalConversions) {
  RegisterDescriptor desc{
      0, 2, "HELLO", 2, false, RegisterValueType::STRING, 0};
  RegisterStore reg(desc);

  std::string str = reg;
  RegisterStoreValue val = reg;
  EXPECT_EQ(str, "  <0x0000> HELLO                            :");
  EXPECT_EQ(val.reg_addr, 0);
  EXPECT_EQ(val.name, "HELLO");
  EXPECT_EQ(val.history.size(), 0);

  reg.front().value = std::vector<uint16_t>({0x3031, 0x3233}); // "0123"
  reg.front().timestamp = 0x1234;
  ++reg;
  str = reg;
  val = reg;
  EXPECT_EQ(str, "  <0x0000> HELLO                            : 0123");
  EXPECT_EQ(val.reg_addr, 0);
  EXPECT_EQ(val.name, "HELLO");
  EXPECT_EQ(val.history.size(), 1);
  EXPECT_EQ(val.history[0].type, RegisterValueType::STRING);
  EXPECT_EQ(val.history[0].value.strValue, "0123");

  reg.front().value = std::vector<uint16_t>({0x3132, 0x3334}); // "1234"
  reg.front().timestamp = 0x1234;
  ++reg;
  str = reg;
  val = reg;
  EXPECT_EQ(str, "  <0x0000> HELLO                            : 0123 1234");
  EXPECT_EQ(val.reg_addr, 0);
  EXPECT_EQ(val.name, "HELLO");
  EXPECT_EQ(val.history.size(), 2);
  EXPECT_EQ(val.history[0].type, RegisterValueType::STRING);
  EXPECT_EQ(val.history[0].value.strValue, "0123");
  EXPECT_EQ(val.history[1].type, RegisterValueType::STRING);
  EXPECT_EQ(val.history[1].value.strValue, "1234");

  nlohmann::json j = val;
  EXPECT_TRUE(j.contains("begin") && j["begin"].is_number_integer());
  EXPECT_TRUE(j.contains("name") && j["name"].is_string());
  EXPECT_TRUE(j.contains("readings") && j["readings"].is_array());
  EXPECT_EQ(j["begin"], 0);
  EXPECT_EQ(std::string(j["name"]), "HELLO");
  EXPECT_EQ(j["readings"].size(), 2);
  EXPECT_TRUE(j["readings"][0].is_object());
  EXPECT_TRUE(j["readings"][1].is_object());
  EXPECT_TRUE(j["readings"][0].contains("type"));
  EXPECT_TRUE(j["readings"][1].contains("type"));
  EXPECT_TRUE(j["readings"][0].contains("value"));
  EXPECT_TRUE(j["readings"][1].contains("value"));
  EXPECT_EQ(std::string(j["readings"][0]["type"]), "string");
  EXPECT_EQ(std::string(j["readings"][1]["type"]), "string");
  EXPECT_EQ(std::string(j["readings"][0]["value"]), "0123");
  EXPECT_EQ(std::string(j["readings"][1]["value"]), "1234");

  nlohmann::json j2 = reg;
  EXPECT_TRUE(j2.contains("begin") && j2["begin"].is_number_integer());
  EXPECT_TRUE(j2.contains("readings") && j2["readings"].is_array());
  EXPECT_EQ(j2["begin"], 0x0);
  EXPECT_EQ(j2["readings"].size(), 2);
  EXPECT_TRUE(
      j2["readings"][0].is_object() && j2["readings"][0].contains("time"));
  EXPECT_TRUE(
      j2["readings"][0].is_object() && j2["readings"][0].contains("data"));
  EXPECT_TRUE(
      j2["readings"][1].is_object() && j2["readings"][1].contains("time"));
  EXPECT_TRUE(
      j2["readings"][1].is_object() && j2["readings"][1].contains("data"));
  EXPECT_EQ(j2["readings"][0]["time"], 0x1234);
  EXPECT_EQ(j2["readings"][1]["time"], 0x1234);
  EXPECT_EQ(std::string(j2["readings"][0]["data"]), "30313233");
  EXPECT_EQ(std::string(j2["readings"][1]["data"]), "31323334");
}