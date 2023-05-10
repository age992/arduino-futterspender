import { Injectable } from '@angular/core';
import { Observable, Observer } from 'rxjs';
import { environment } from 'src/environments/environment';

@Injectable({
  providedIn: 'root',
})
export class WebsocketService {
  private ws!: WebSocket;
  constructor() {
    this.ws = new WebSocket(environment.wsUrl);
    this.ws.onmessage = (e) => {
      console.log(e.data);
    };
  }
}
