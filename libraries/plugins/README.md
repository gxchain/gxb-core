# GXChain Plugins

The plugins are a collection of tools that brings new functionality without the need of modifications in the consensus.

The main source of I/O of the GXChain blockchain is the API. Plugins are a more powerful alternative to build more complex developments for when the current API is not enough.

Plugins are optional to run by node operator according to their needs. However, all plugins here will be compiled.

# Available Plugins

Folder                             | Name                     | Description                                                                 | Category       | Status        | SpaceID
-----------------------------------|--------------------------|-----------------------------------------------------------------------------|----------------|---------------|--------------|
[account_history](account_history) | Account History          | Save account history data                                                   | History        | Stable        | 4
[debug_witness](debug_witness)     | Debug Witness            | Run "what-if" tests                                                         | Debug          | Stable        |
[snapshot](snapshot)               | Snapshot                 | Get a json of all objects in blockchain at a specificed time or block       | Debug          | Stable        |
[witness](witness)                 | Witness                  | Generate and sign blocks                                                    | Block producer | Stable        |
