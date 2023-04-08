import { NgModule } from '@angular/core';
import { RouterModule, Routes } from '@angular/router';
import { DashboardComponent } from './dashboard/dashboard.component';
import { SettingsComponent } from './settings/settings.component';
import { SchedulesComponent } from './schedules/schedules.component';
import { HistoryComponent } from './history/history.component';

const routes: Routes = [
  { path: '', redirectTo: '/dashboard', pathMatch: 'full' },
  {
    path: 'dashboard',
    component: DashboardComponent,
    data: { animation: 'dashboard' },
  },
  {
    path: 'schedules',
    component: SchedulesComponent,
    data: { animation: 'schedules' },
  },
  {
    path: 'settings',
    component: SettingsComponent,
    data: { animation: 'settings' },
  },
  {
    path: 'history',
    component: HistoryComponent,
    data: { animation: 'history' },
  },
];

@NgModule({
  imports: [RouterModule.forRoot(routes)],
  exports: [RouterModule],
})
export class AppRoutingModule {}
