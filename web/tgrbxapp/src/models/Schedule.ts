import { EScheduleMode } from 'src/lib/EScheduleMode';

export interface Schedule {
  ID?: number;
  CreatedOn?: number;
  Name?: string;
  Selected?: boolean;
  Active?: boolean;
  Mode?: EScheduleMode;
  Daytimes?: number[];
  MaxTimes?: number;
  MaxTimesStartTime?: number;
  OnlyWhenEmpty?: boolean;
}
