import { EScheduleMode } from 'src/app/lib/EScheduleMode';
import { Schedule } from 'src/models/Schedule';

const d1 = new Date('1970-01-01T06:00:00.000Z').getTime();
const d2 = new Date('1970-01-01T12:30:00.000Z').getTime();
const d3 = new Date('1970-01-01T19:30:00.000Z').getTime();

export const schedules: Schedule[] = [
  {
    ID: 1,
    Name: 'Fritzchen Ferien',
    Active: false,
    Selected: false,
    Mode: EScheduleMode.Max,
    OnlyWhenEmpty: true,
    MaxTimes: 3,
    MaxTimesStartTime: d1,
  },
  {
    ID: 2,
    Name: 'Fritzchen Normal',
    Active: true,
    Selected: true,
    Mode: EScheduleMode.FixedDaytime,
    OnlyWhenEmpty: false,
    Daytimes: [d1, d2, d3],
  },
];

//export const schedules: Schedule[] = [];
