import { EScheduleMode } from 'src/app/lib/EScheduleMode';

export interface Schedule {
  ID: Number;
  Name: string;
  Selected: boolean;
  Active: boolean;
  Mode: EScheduleMode;
  Daytimes?: Date[];
  MaxTimes?: Number;
  OnlyWhenEmpty: boolean;
}
