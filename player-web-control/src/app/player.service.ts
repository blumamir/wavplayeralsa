import { Injectable } from '@angular/core';
import { HttpClient, HttpHeaders, HttpErrorResponse } from '@angular/common/http';
import { Observable } from 'rxjs';
import { map, catchError } from 'rxjs/operators';

@Injectable({
  providedIn: 'root'
})
export class PlayerService {

  constructor(private http: HttpClient) { }

  readonly httpOptions = {
    headers: new HttpHeaders({ 'Content-Type': 'application/json' })  
  };

  queryFiles(): Observable<string[]> {
    return this.http.get("/api/available-files").pipe(
      map(json => json as string[])
    );
  }

  stopAudio(): Observable<string> {

    const headers = new HttpHeaders().set('Content-Type', 'application/json');

    var reqJson = {}; // empty obejct means 'stop'
    return this.http.put("/api/current-song", reqJson, {headers, responseType: 'text'}).pipe(
      catchError((err: HttpErrorResponse) => {throw err.error})
    );
  }

  playAudio(fileId: string, startOffsetMs: number): Observable<string> {

    const headers = new HttpHeaders().set('Content-Type', 'application/json');

    var reqJson = {"file_id": fileId, "start_offset_ms": startOffsetMs};
    return this.http.put("/api/current-song", reqJson, {headers, responseType: 'text'}).pipe(
      catchError((err: HttpErrorResponse) => {throw err.error})
    );
  }

}
