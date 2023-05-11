import { Component, OnInit } from '@angular/core';
import { MachineStatus } from 'src/models/MachineStatus';
import { Schedule } from 'src/models/Schedule';
import { ScheduleService } from 'src/services/schedule/schedule.service';
import { StatusService } from 'src/services/status/status.service';
import { EScheduleMode } from 'src/lib/EScheduleMode';
import { getTimestamp } from 'src/lib/DateConverter';
import { SettingsService } from 'src/services/settings/settings.service';
import { Settings } from 'src/models/Settings';

@Component({
  selector: 'dashboard',
  templateUrl: './dashboard.component.html',
  styleUrls: ['./dashboard.component.scss'],
})
export class DashboardComponent implements OnInit {
  public EScheduleMode: typeof EScheduleMode = EScheduleMode;
  public getTimestamp = getTimestamp;

  public MachineStatus: MachineStatus | null = null;
  public LoadingStatus: boolean = false;
  public Connected: boolean = true;

  public settings: Settings | null = null;
  public FetchingSettings = false;

  public currentSchedule: Schedule | null = null;
  public nextFeedingTime: number | null = null;
  public FetchingSchedule: boolean = false;
  public UpdatingActivity: boolean = false;

  constructor(
    public statusService: StatusService,
    private scheduleService: ScheduleService,
    private settingsService: SettingsService
  ) {}

  ngOnInit(): void {
    this.statusService.MachineStatus.subscribe((m: MachineStatus | null) => {
      this.MachineStatus = m;
    });
    this.statusService.Loading.subscribe((l) => {
      this.LoadingStatus = l;
    });
    this.statusService.Connected.subscribe((c) => {
      this.Connected = c;
    });

    this.settingsService.Settings.subscribe((s) => (this.settings = s));
    this.settingsService.FetchingSettings.subscribe(
      (f) => (this.FetchingSettings = f)
    );

    this.scheduleService.Schedules.subscribe((s) => {
      if (!s || s.length == 0) {
        return;
      }

      for (let schedule of s) {
        if (schedule.Selected) {
          this.currentSchedule = schedule;
          if (schedule.Daytimes) {
            const now = new Date();
            schedule.Daytimes = schedule.Daytimes?.map((d) => {
              const date = new Date(d);
              date.setFullYear(now.getFullYear());
              date.setMonth(now.getMonth());
              date.setDate(now.getDate());
              return date.getTime();
            });
            schedule.Daytimes.sort();
            let index = schedule.Daytimes.findIndex((d) => d > now.getTime());
            const nextTime =
              index > -1 ? schedule.Daytimes[index] : schedule.Daytimes[0];
            this.nextFeedingTime = nextTime;
          }
          break;
        }
      }
    });

    this.scheduleService.FetchingSchedules.subscribe((l) => {
      this.FetchingSchedule = l;
    });
  }

  toggleScheduleActive = (active: boolean) => {
    /*const toggledSchedule: Schedule = {
      ...this.currentSchedule!,
      Active: active,
    };*/
    this.UpdatingActivity = true;
    this.scheduleService
      .toggleScheduleActive(this.currentSchedule!, active)
      .subscribe((s) => {
        this.UpdatingActivity = false;
      });
  };

  protected get containerLoadPercentage() {
    if (!this.MachineStatus) {
      return 0;
    }
    return (this.MachineStatus.ContainerLoad / 1000) * 100;
  }

  protected get plateLoadPercentage() {
    if (!this.MachineStatus) {
      return 0;
    }
    return (this.MachineStatus.PlateLoad / 100) * 100;
    // return this.MachineStatus!.PlateLoad / this.settings!.PlateFilling as number;
  }
}
