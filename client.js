const { google } = require("googleapis");

class GdriveClient {
  constructor() {
    const auth = new google.auth.OAuth2(
      process.env.CLIENT_ID, 
      process.env.CLIENT_SECRET, 
    );

    auth.setCredentials({
      refresh_token: process.env.REFRESH_TOKEN
    });


    this.drive = google.drive({ version: "v3", auth });
    this.sheets = google.sheets({ version: "v4", auth });
  }

  async listFiles(searchQuery) {
    const query = (pageToken = null) => {
      return new Promise(resolve => {
        this.drive.files.list({
          q: searchQuery,
          fields: 'files(id, name)',
        }, async (err, res) => {
          if (err) return console.log('The API returned an error: ' + err);
          pageToken = res.nextPageToken;
          const files = res.data.files;
          if (pageToken) {
            files.concat(await query(pageToken))
          }
          resolve(files);
        });
      })
    }

    return await query();
  }

  findFile(fileId) {
    return new Promise(resolve => {
      this.drive.files.get({ fileId }, (err, res) => {
        if (err) return console.log('The API returned an error: ' + err);
        resolve(res.data);
      });
    })
  }

  addRow(fileId, row) {
    return new Promise(resolve => {
      this.sheets.spreadsheets.values.update({
        spreadsheetId: fileId,
        valueInputOption: "RAW",
        range: "", // TODO
        resource: {
          "values": [row]
        }
      })
    })
  }

  getColumnData(fileId, columnIndex) {
    const range = this.getColumn(columnIndex);
    return new Promise(resolve => {
      this.sheets.spreadsheets.values.get({
        range,
        spreadsheetId: fileId,
      }, (err, response) => {
        if (err) return console.log('The API returned an error: ' + err);
        resolve(response.data.values.map(row => row[0]));
      })
    })
  }

  getSheet(spreadsheetId) {
    return new Promise(resolve => {
      this.sheets.get({ spreadsheetId }, (err, res) => {
        if (err) return console.log('The API returned an error: ' + err);
        resolve(res.data.sheet);
      });
    });
  }

  getRow(rowIndex) {
    return `A${rowIndex + 1}:AAA${rowIndex + 1}`
  }

  getColumn(columnIndex) {
    const letter = String.fromCharCode(97 + columnIndex);
    return `${letter}2:${letter}10000`
  }
}

exports.GdriveClient = GdriveClient;
