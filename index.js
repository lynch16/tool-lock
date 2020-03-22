const { client } = require("./db_client");

const promiseWrapper = (promiseLikeFn) => {
  return new Promise(resolve => {
    promiseLikeFn().then(() => {
      resolve();
      process.exit();
    })
  })
}

exports.downloadUsers = () => promiseWrapper(() => client.download(process.env.FILE_ID));
exports.verifyUser = (event) => promiseWrapper(() => client.verify(process.env.FILE_ID, event.queryStringParameters.cardId));
exports.addUser = (event) => promiseWrapper(() => client.verify(process.env.FILE_ID, event.queryStringParameters.cardId));
