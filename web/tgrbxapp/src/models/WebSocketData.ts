import { MachineStatus } from './MachineStatus';
import { ScaleData } from './ScaleData';
import { Schedule } from './Schedule';

export interface WebSocketData {
  history: HistoryData;
  status: MachineStatus;
}

export interface HistoryData {
  schedules: Schedule[];
  events: Event[];
  scaleData: ScaleData[];
}
