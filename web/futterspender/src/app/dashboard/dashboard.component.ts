import { Component, OnInit } from '@angular/core';
import { MachineStatus } from 'src/models/MachineStatus';
import { Schedule } from 'src/models/Schedule';
import { ScheduleService } from 'src/services/schedule/schedule.service';
import { StatusService } from 'src/services/status/status.service';
import { EScheduleMode } from 'src/app/lib/EScheduleMode';

@Component({
  selector: 'dashboard',
  templateUrl: './dashboard.component.html',
  styleUrls: ['./dashboard.component.scss'],
})
export class DashboardComponent implements OnInit {
  public EScheduleMode: typeof EScheduleMode = EScheduleMode;

  public MachineStatus: MachineStatus | null = null;
  public LoadingStatus: boolean = false;
  public Connected: boolean = true;

  public currentSchedule: Schedule | null = null;
  public nextFeedingTime: Date | null = null;
  public LoadingSchedule: boolean = false;
  public UpdatingActivity: boolean = false;

  constructor(
    private statusService: StatusService,
    private scheduleService: ScheduleService
  ) {}

  ngOnInit(): void {
    this.statusService.MachineStatus.subscribe((m) => {
      this.MachineStatus = m;
    });
    this.statusService.Loading.subscribe((l) => {
      this.LoadingStatus = l;
    });
    this.statusService.Connected.subscribe((c) => {
      this.Connected = c;
    });

    this.scheduleService.Schedules.subscribe((s) => {
      for (let schedule of s) {
        if (schedule.Selected) {
          this.currentSchedule = schedule;
          if (schedule.Daytimes) {
            const now = new Date();
            schedule.Daytimes = schedule.Daytimes?.map((d) => {
              d.setFullYear(now.getFullYear());
              d.setMonth(now.getMonth());
              d.setDate(now.getDate());
              return d;
            });
            schedule.Daytimes.sort((d1, d2) => d1.getTime() - d2.getTime());
            let index = schedule.Daytimes.findIndex(
              (d) => d.getTime() > now.getTime()
            );
            const nextTime =
              index > -1 ? schedule.Daytimes[index] : schedule.Daytimes[0];
            this.nextFeedingTime = nextTime;
          }
          break;
        }
      }
    });

    this.scheduleService.Loading.subscribe((l) => {
      this.LoadingSchedule = l;
    });
  }

  toggleScheduleActive = (active: boolean) => {
    const toggledSchedule: Schedule = {
      ...this.currentSchedule!,
      Active: active,
    };
    this.UpdatingActivity = true;
    this.scheduleService.updateSchedule(toggledSchedule).subscribe((s) => {
      this.currentSchedule = toggledSchedule;
      this.UpdatingActivity = false;
    });
  };
}
