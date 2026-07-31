#include <gtest/gtest.h>

#include "engine/Event.h"
#include "services/Services.h"
#include "services/console/CommandContext.h"

namespace parus
{
    TEST(CommandContext, WriteAggregatesOutput)
    {
        Services::registerService<EventSystem>(std::make_shared<EventSystem>());

        CommandContext context("dummy");
        context.write("first");
        context.write("second");

        EXPECT_EQ(context.output(), "first\nsecond");
    }

    TEST(CommandContext, WriteFiresProgressEvent)
    {
        Services::registerService<EventSystem>(std::make_shared<EventSystem>());

        std::string capturedCommand;
        std::string capturedMessage;
        REGISTER_EVENT(EventType::EVENT_CONSOLE_COMMAND_PROGRESS,
            [&](const std::string command, const std::string message)
        {
            capturedCommand = command;
            capturedMessage = message;
        });

        CommandContext context("look at me");
        context.write("progress line");

        EXPECT_EQ(capturedCommand, "look at me");
        EXPECT_EQ(capturedMessage, "progress line");
    }
}
