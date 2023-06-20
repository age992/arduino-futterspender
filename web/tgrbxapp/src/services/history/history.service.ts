import { Injectable } from '@angular/core';
import { WebsocketService } from '../websocket/websocket.service';
import { Scale, ScaleData } from 'src/models/ScaleData';
import { Schedule } from 'src/models/Schedule';
import { HistoryData, WebSocketData } from 'src/models/WebSocketData';
import { BehaviorSubject } from 'rxjs';
import { getTodayUnixSeconds } from 'src/lib/DateConverter';
import { EventData, EventType } from 'src/models/Event';

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
    //
    const today = getTodayUnixSeconds();
    const firstContainerData: ScaleData = {
      ID: 0,
      CreatedOn: today,
      ScaleID: Scale.Container,
      Value: 3,
    };
    const firstPlateData: ScaleData = {
      ID: 0,
      CreatedOn: today,
      ScaleID: Scale.Plate,
      Value: 300,
    };
    this.historyDataInternal.scaleData.push(firstContainerData, firstPlateData);

    const feedEvent: EventData = {
      ID: 0,
      CreatedOn: 86400 / 12,
      Type: EventType.Feed,
      Value: 'Gefüttert!',
    };
    this.historyDataInternal.events.push(feedEvent);
    //

    websocketService.webSocketData.subscribe((w: WebSocketData | null) => {
      const newHistoryData = w?.history;
      if (!!newHistoryData) {
        if (!!newHistoryData.events && newHistoryData.events.length > 0) {
          this.historyDataInternal.events.push(...newHistoryData.events);
        }
        if (!!newHistoryData.scaleData && newHistoryData.scaleData.length > 0) {
          this.historyDataInternal.scaleData.push(...newHistoryData.scaleData);
        }
        if (!!newHistoryData.schedules && newHistoryData.schedules.length > 0) {
          this.historyDataInternal.schedules.push(...newHistoryData.schedules);
        }
        this.HistoryData.next(newHistoryData);
      }
    });
  }
}
