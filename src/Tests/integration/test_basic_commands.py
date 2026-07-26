import pytest
from client import ByteForgeClient


@pytest.fixture
def client():
    c = ByteForgeClient()
    yield c
    c.close()


def test_insert_get_delete(client):
    assert client.insert("volt", "3333") == "Insert command was successful"
    assert client.get("volt") == "Get command was successful: 3333"
    assert client.delete("volt") == "Delete command was successful"


def test_get_after_delete(client):
    client.insert("volt", "3333")
    client.get("volt")
    client.delete("volt")

    response = client.get("volt")
    assert response == "Get command was successful but key dont exist"
