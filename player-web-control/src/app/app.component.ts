import { Component, OnInit } from '@angular/core';
import { PlayerService } from './player.service'

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.css']
})
export class AppComponent implements OnInit {

  ngOnInit(): void {
    this.playerService.queryFiles().subscribe(
      filesArr => {
        this.nodes = [];
        filesArr.forEach(fileId => this.nodes.push({id: fileId, name: fileId}));
      }
    );
  }

  constructor(private playerService: PlayerService) { }

  nodes = [];
  options = {};

  fileToPlay: string;

  responseText: string;

  onActivate(event) : void {
    this.fileToPlay = event.node.id;
  }

  stop() :void {
    this.playerService.stopAudio().subscribe( {
      next: msg => {this.responseText = msg},
      error: err_msg => {this.responseText = err_msg}
    })
  }

  play() :void {
    this.playerService.playAudio(this.fileToPlay, 0).subscribe( {
      next: msg => {this.responseText = msg},
      error: err_msg => {this.responseText = err_msg}
    })
  }

}
