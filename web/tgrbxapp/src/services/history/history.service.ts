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
  private historyDataInternal: HistoryData = {
    schedules: [],
    events: [],
    scaleData: [],
  };

  public HistoryData = new BehaviorSubject<HistoryData>(
    this.historyDataInternal
  );

  constructor(private websocketService: WebsocketService) {
    websocketService.webSocketData.subscribe((w: WebSocketData | null) => {
      const newHistoryData = w?.history;
      if (!!newHistoryData) {
        if (newHistoryData.events && newHistoryData.events.length > 0) {
          this.historyDataInternal.events.push(...newHistoryData.events);
        }
        if (newHistoryData.scaleData && newHistoryData.scaleData.length > 0) {
          this.historyDataInternal.scaleData.push(...newHistoryData.scaleData);
        }
        if (newHistoryData.schedules && newHistoryData.schedules.length > 0) {
          this.historyDataInternal.schedules.push(...newHistoryData.schedules);
        }
        this.HistoryData.next(newHistoryData);
      }
    });
  }
}
