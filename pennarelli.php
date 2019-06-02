<?php
  $link = new mysqli("localhost","root","utente","DBPennarelli");
  if($mysqli->connect_errno){
    exit();
  }
  $action = $_GET["action"];
  if($action !== "take" and $action !== "file") exit();
  $parametri["file"] = array("codM","value");
  $parametri["take"] = array("codM","prof","datetime","coloreP");
  foreach($parametri[$action] as $name){
    if(!isset($_GET[$name]) exit();
  } 
  $codM = $_GET["codM"];
  switch($action) {
      case "file":
          $colori = explode(":", $_GET["value"]);
          $array = ["rosso" => $colori[0], "blu" => $colori[1], "verde" => $colori[2], "nero" => $colori[3]];
          $finiti = "";
          foreach ($array as $colore => $rimanenti) {
              if ($rimanenti === 0) $finiti .= (strlen($finiti) === 0 ? $colore : (", " . $colore));
          }
          if (strlen($finiti) !== 0) {
              $res1 = $link->query("SELECT piano FROM Macchina WHERE codM='$codM'");
              if ($res->num_rows === 0) exit();
              $piano = $res->fetch_array()[0];
              $mailAddress = file_get_contents("mailgestore.txt");
              $testo = "I pennarelli dei seguenti colori sono finiti: $finiti nella macchinetta $codM al piano $piano";
              mail($mailAddress, "Rifornimento pennarelli", $testo);
          }
          break;
      case "take":
          $datetime = date("Y-m-d h:i:s");
          $res = $link->query("INSERT INTO Utilizza VALUES('$datetime','" . $_GET["coloreP"] . "','$codM','" . $_GET["prof"] . "')");
          if (!($res instanceof mysqli_result)) exit();
      case "start":
          $checksum  = "";
          if($res = $link->query("select codTessera from Professore") ){
              while ($row=$res->fetch_row()){
                  $checksum .= $row[0];
              }
          }
          $checksum=md5($checksum);
          if(isset ($_GET["md5"]) &&($_GET["md5"]===$checksum)) die($checksum."\n");
          else {
              $query="SELECT codTessera FROM Professori";
              $res=$link->query($query);
              $codici="\n";
              while ($row=$res->fetch_row()){
                  $codici.=$row[0]."\n";
              }
              echo $checksum.$codici;
          }
  }
	//						   R:V:N:B
	//GET pennarelli.php?action=file&codM=23546&value="1:2:3:4"
	//GET pennarelli.php?action=take&codM=25345664&prof=343456&date="data"&time="time"&coloreP="colore"



/*
//ARDUINO
bufferTake.txt
prof=343456&date="data"&time="time"&coloreP="colore";prof=343456&date="data"&time="time"&coloreP="colore";prof=343456&date="data"&time="time"&coloreP="colore"

bufferFile.txt
0 => non trasmettere
1 => recupera codici dal file numeroPennarelli.txt ed invia

numeroPennarelli.txt
1234

tessere.txt (deve essere aggiornato 1 volta al giorno)
cod: AA000AA;

idMacchina.txt
codMacchina: 24e1235q, pos: A1


-->*/