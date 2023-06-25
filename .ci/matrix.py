import os
import time
import simplematrixbotlib as botlib
import argparse

parser = argparse.ArgumentParser(
                    prog='Matrix bot',
                    description='Post a message to the Matrix channel',
                    epilog='Done')
parser.add_argument('-m', '--message', help="The message to post")
parser.add_argument('-s', '--server', help="URL of the homeserver")
parser.add_argument('-u', '--user', help="Username of the bot")
parser.add_argument('-t', '--token', help="Access token of the bot")
parser.add_argument('-r', '--room', help="ID of the room, like `!xxxx:matrix.org`")
args = parser.parse_args()

creds = botlib.Creds(homeserver=args.server, username=args.user, access_token=args.token)
bot = botlib.Bot(creds=creds)

@bot.listener.on_startup
async def send_message(joined_room_id: str) -> None:
    if args.room and args.room != joined_room_id:
        return

    message = f"""{args.message}"""

    await bot.api.send_markdown_message(
        room_id=joined_room_id,
        message=message,
        msgtype="m.notice")

    exit()

bot.run()
