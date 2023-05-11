import { Injectable, OnDestroy } from '@angular/core';
import {
  BehaviorSubject,
  Observable,
  Observer,
  delay,
  of,
  retry,
  retryWhen,
  switchMap,
} from 'rxjs';
import { environment } from 'src/environments/environment';
import { webSocket, WebSocketSubject } from 'rxjs/webSocket';

@Injectable({
  providedIn: 'root',
})
export class WebsocketService implements OnDestroy {
  public webSocketData = new BehaviorSubject<any | null>(null);
  public webSocketConnected = new BehaviorSubject(false);

  private ws!: WebSocket;
  private RETRY_SECONDS = 5;

  constructor() {
    this.ws = new WebSocket(environment.wsUrl);
    this.ws.onopen = () => {
      console.log('WS Connection opened');
      this.webSocketConnected.next(true);
    };
    this.ws.onerror = (e) => {
      console.log('WebSocket error: ', e);
      this.webSocketConnected.next(false);
    };
    this.ws.onmessage = (ev) => {
      this.webSocketData.next(JSON.parse(ev.data));
    };
  }

  ngOnDestroy() {
    console.log('Destroy Websocket service');
    this.ws.close();
  }
}
