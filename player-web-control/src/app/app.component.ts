import { Component } from '@angular/core';
import { PlayerService } from './player.service'

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.css']
})
export class AppComponent {

  constructor(private playerService: PlayerService) { }

  nodes = [
    {
      id: 'toccata.wav',
      name: 'toccata.wav',
    },
    {
      id: 'mantequilla.wav',
      name: 'mantequilla.wav',
    },
    {
      id: 'dorothy.wav',
      name: 'dorothy.wav',
    },
    {
      id: 'mumminim.wav',
      name: 'mumminim.wav',
    }

  ];
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
