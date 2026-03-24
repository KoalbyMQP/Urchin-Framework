from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class Session:
    session_id: int
    client_name: str


class SessionManager:
    def __init__(self) -> None:
        self._next_session_id = 1
        self._sessions: dict[int, Session] = {}

    def create(self, client_name: str) -> Session:
        session = Session(session_id=self._next_session_id, client_name=client_name)
        self._sessions[session.session_id] = session
        self._next_session_id += 1
        return session

    def remove(self, session_id: int) -> None:
        self._sessions.pop(session_id, None)

    def require(self, session_id: int) -> Session:
        session = self._sessions.get(session_id)
        if session is None:
            raise KeyError(f"Unknown session_id {session_id}")
        return session

    def exists(self, session_id: int) -> bool:
        return session_id in self._sessions
