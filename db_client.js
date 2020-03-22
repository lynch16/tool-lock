const { WebClient } = require('@slack/web-api');
const mdb = require("mongodb");
const mongo = mdb.MongoClient;
const DriveClient = require("./client").GdriveClient;

const driveClient = new DriveClient();

async function slackAlert(message) {
  const slack = new WebClient(process.env.SLACK_TOKEN);
  await slack.chat.postMessage({ 
    channel: process.env.CHANNEL_ID,
    text: message,
    username: 'Checkout Monitor Bot',
    icon_emoji: ':key:'
  });
}

const client = {
  // Verify User is in collection as fallback
  verify: (fileId, cardUID) => {
    return new Promise(async (resolve) => { 
      const ids = await driveClient.getColumnData(fileId, 0);
      mongo.connect(process.env.DB_URI, { useNewUrlParser: true }, (error, client) => {
        if (error) return slackAlert(error);
        client.db(process.env.DB_NAME).collection("cards").findOne({ "uid": mdb.ObjectId(cardUID) }).then(async (card, err) => {
          if (err) return slackAlert(err);
          const exists = ids.includes(card && card.member_id);
          await slackAlert(`Card ${cardUID} exists in ${fileId}? ${exists}`);
          resolve(exists);
        });
      });
    });
  },
  // Download authorized user IDs from list
  download: (fileId) => {
    return new Promise(async (resolve) => {
      const ids = await driveClient.getColumnData(fileId, 0);
      mongo.connect(process.env.DB_URI, { useNewUrlParser: true }, (error, client) => {
        if (error) return slackAlert(error);
        client.db(process.env.DB_NAME).collection("cards").find({ "$or": 
          ids.map(memberId =>  ({ "member_id": mdb.ObjectId(memberId) }))
        }).toArray(async (err, cards) => {
          if (err) return slackAlert(err);
          const cardUIDs = cards.map(card => card.uid);
          await slackAlert(`Card UIDs downloaded: ${cards.length}`);
          resolve(cardUIDs);
        })
      });
    })
  },
  // Add authorized users to list
  add: (fileId, cardUID) => {
    return new Promise(async (resolve) => {
      mongo.connect(process.env.DB_URI, { useNewUrlParser: true }, (error, client) => {
        if (error) return slackAlert(error);
        client.db(process.env.DB_NAME).collection("cards").findOne({ "uid": mdb.ObjectId(cardUID) }).then((card, err) => {
          if (err) return slackAlert(err);
          card && driveClient.addRow(fileId, [card.member_id, card.holder]).then(async (resp) => {
            await slackAlert(`Card ${cardUID} added to ${fileId}`);
            resolve(resp);
          });
        })
      });
    })
  }
}

exports.client = client;