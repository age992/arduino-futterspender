import { Injectable } from '@angular/core';
import { WebsocketService } from '../websocket/websocket.service';
import { ScaleData } from 'src/models/ScaleData';
import { Schedule } from 'src/models/Schedule';
import { HistoryData, WebSocketData } from 'src/models/WebSocketData';
import { BehaviorSubject } from 'rxjs';

@Injectable({
  providedIn: 'root',
})
export class HistoryService {
  public History: HistoryData = {
    schedules: [],
    events: [],
    scaleData: [],
  };

  constructor(private websocketService: WebsocketService) {
    websocketService.webSocketData.subscribe((w: WebSocketData | null) => {
      const d = w?.history;
      if (d?.events && d.events.length > 0) {
        this.History.events.push(...d.events);
      }
      if (d?.scaleData && d.scaleData.length > 0) {
        this.History.scaleData.push(...d.scaleData);
      }
      if (d?.schedules && d.schedules.length > 0) {
        this.History.schedules.push(...d.schedules);
      }
    });
  }
}
